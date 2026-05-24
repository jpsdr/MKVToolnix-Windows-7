/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   subtitle helper

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <QRegularExpression>

#include "common/endian.h"
#include "common/mime.h"
#include "common/mm_proxy_io.h"
#include "common/mm_text_io.h"
#include "common/qt.h"
#include "common/strings/formatting.h"
#include "common/strings/parsing.h"
#include "common/strings/utf8.h"
#include "input/subtitles.h"
#include "merge/file_status.h"
#include "merge/input_x.h"
#include "merge/generic_reader.h"
#include "merge/packet_extensions.h"

// ------------------------------------------------------------

subtitles_c::subtitles_c(std::string const &file_name,
                         int64_t track_id)
  : current{entries.end()}
  , m_cc_utf8{charset_converter_c::init("UTF-8")}
  , m_invalid_utf8_warned{g_identifying}
  , m_file_name{file_name}
  , m_track_id{track_id}
{
}

void
subtitles_c::add_maybe(int64_t start,
                       int64_t end,
                       unsigned int number,
                       std::string const &subs) {
  if (end <= start) {
    ++m_num_skipped;
    mxdebug_if(m_debug, fmt::format("skipping entry no. {0} {1} — {2} content: {3}\n", number, mtx::string::format_timestamp(start), mtx::string::format_timestamp(end), subs));
    return;
  }

  add(start, end, number - m_num_skipped, subs);
}

void
subtitles_c::set_charset_converter(charset_converter_cptr const &cc_utf8) {
  if (cc_utf8)
    m_cc_utf8 = cc_utf8;

  else {
    m_cc_utf8  = g_cc_local_utf8;
    m_try_utf8 = true;
  }
}

std::string
subtitles_c::recode(std::string const &s,
                     uint32_t replacement_marker) {
  if (m_try_utf8 && !mtx::utf8::is_valid(s))
    m_try_utf8 = false;

  auto recoded = m_try_utf8 ? s : m_cc_utf8->utf8(s);

  if (mtx::utf8::is_valid(recoded))
    return recoded;

  if (!m_invalid_utf8_warned) {
    m_invalid_utf8_warned = true;
    mxwarn_tid(m_file_name, m_track_id, fmt::format(FY("This text subtitle track contains invalid 8-bit characters outside valid multi-byte UTF-8 sequences. Please specify the correct encoding for this track.\n")));
  }

  return mtx::utf8::fix_invalid(recoded, replacement_marker);
}

void
subtitles_c::process(generic_packetizer_c *p) {
  if (empty() || (entries.end() == current))
    return;

  packet_cptr packet(new packet_t(memory_c::borrow(current->subs), current->start, current->end - current->start));
  packet->extensions.push_back(packet_extension_cptr(new subtitle_number_packet_extension_c(current->number)));
  p->process(packet);
  ++current;
}

// ------------------------------------------------------------

#define SRT_RE_VALUE          "\\s*(-?)\\s*(\\d+)"
#define SRT_RE_TIMESTAMP      SRT_RE_VALUE ":" SRT_RE_VALUE ":" SRT_RE_VALUE "(?:[,\\.:]" SRT_RE_VALUE ")?"
#define SRT_RE_TIMESTAMP_LINE "^" SRT_RE_TIMESTAMP "\\s*[\\-\\s]+>\\s*" SRT_RE_TIMESTAMP "\\s*"
#define SRT_RE_COORDINATES    "([XY]\\d+:\\d+\\s*){4}\\s*$"

bool
srt_parser_c::probe(mm_text_io_c &io) {
  try {
    io.setFilePointer(0);
    std::string s;
    do {
      s = io.getline(10);
      mtx::string::strip(s);
    } while (s.empty());

    int64_t dummy;
    if (!mtx::string::parse_number(s, dummy))
      return false;

    s = io.getline(100);
    QRegularExpression timestamp_re{SRT_RE_TIMESTAMP_LINE};
    if (!Q(s).contains(timestamp_re))
      return false;

    s = io.getline();
    io.setFilePointer(0);

  } catch (...) {
    return false;
  }

  return true;
}

srt_parser_c::srt_parser_c(mm_text_io_cptr const &io,
                           const std::string &file_name,
                           int64_t track_id)
  : subtitles_c{file_name, track_id}
  , m_io(io)
  , m_coordinates_warning_shown(false)
{
}

void
srt_parser_c::parse() {
  QRegularExpression timestamp_re{SRT_RE_TIMESTAMP_LINE};
  QRegularExpression number_re{"^\\d+$"};
  QRegularExpression coordinates_re{SRT_RE_COORDINATES};

  int64_t start                  = 0;
  int64_t end                    = 0;
  int64_t previous_start         = 0;
  bool timestamp_warning_printed = false;
  parser_state_e state           = STATE_INITIAL;
  int line_number                = 0;
  unsigned int subtitle_number   = 0;
  unsigned int timestamp_number  = 0;
  std::string subtitles;

  m_io->setFilePointer(0);

  while (1) {
    std::string s;
    if (!m_io->getline2(s))
      break;

    s = recode(s);
    auto unstripped_line = s;
    mtx::string::strip_back(s);

    line_number++;

    mxdebug_if(m_debug, fmt::format("line {0} state {1} content »{2}«\n", line_number, state == STATE_INITIAL ? "initial" : state == STATE_TIME ? "time" : state == STATE_SUBS ? "subs" : "subs-or-number", s));

    if (s.empty()) {
      if ((STATE_INITIAL == state) || (STATE_TIME == state))
        continue;

      state = STATE_SUBS_OR_NUMBER;

      if (!subtitles.empty())
        subtitles += "\n";

      continue;
    }

    if (STATE_INITIAL == state) {
      if (!Q(s).contains(number_re)) {
        mxwarn_tid(m_file_name, m_track_id, fmt::format(FY("Error in line {0}: expected subtitle number and found some text.\n"), line_number));
        break;
      }
      state = STATE_TIME;
      mtx::string::parse_number(s, subtitle_number);

    } else if (STATE_TIME == state) {
      auto matches = timestamp_re.match(Q(s));
      if (!matches.hasMatch()) {
        mxwarn_tid(m_file_name, m_track_id, fmt::format(FY("Error in line {0}: expected a SRT timestamp line but found something else. Aborting this file.\n"), line_number));
        break;
      }

      int64_t s_h = 0, s_min = 0, s_sec = 0, s_ns = 0, e_h = 0, e_min = 0, e_sec = 0, e_ns = 0;

      //      1       2         3    4          5   6                  7   8
      // "\\s*(-?)\\s*(\\d+):\\s(-?)*(\\d+):\\s*(-?)(\\d+)(?:[,\\.]\\s*(-?)(\\d+))?"

      mtx::string::parse_number(to_utf8(matches.captured( 2)), s_h);
      mtx::string::parse_number(to_utf8(matches.captured( 4)), s_min);
      mtx::string::parse_number(to_utf8(matches.captured( 6)), s_sec);
      mtx::string::parse_number(to_utf8(matches.captured(10)), e_h);
      mtx::string::parse_number(to_utf8(matches.captured(12)), e_min);
      mtx::string::parse_number(to_utf8(matches.captured(14)), e_sec);

      std::string s_rest = to_utf8(matches.captured( 8));
      std::string e_rest = to_utf8(matches.captured(16));

      auto neg_calculator = [&matches](auto start_idx) -> auto {
        int64_t neg = 1;
        for (auto idx = 0; idx < 4; ++idx)
          if (matches.captured(start_idx + (idx * 2)) == Q("-"))
            neg *= -1;
        return neg;
      };

      int64_t s_neg = neg_calculator(1);
      int64_t e_neg = neg_calculator(9);

      if (Q(s).contains(coordinates_re) && !m_coordinates_warning_shown) {
        mxwarn_tid(m_file_name, m_track_id,
                   Y("This file contains coordinates in the timestamp lines. "
                     "Such coordinates are not supported by the Matroska SRT subtitle format. "
                     "The coordinates will be removed automatically.\n"));
        m_coordinates_warning_shown = true;
      }

      // The previous entry is done now. Append it to the list of subtitles.
      if (!subtitles.empty())
        add_maybe(start, end, timestamp_number, subtitles);

      while (s_rest.length() < 9)
        s_rest += "0";
      if (s_rest.length() > 9)
        s_rest.erase(9);

      while (e_rest.length() < 9)
        e_rest += "0";
      if (e_rest.length() > 9)
        e_rest.erase(9);

      mtx::string::parse_number(s_rest, s_ns);
      mtx::string::parse_number(e_rest, e_ns);

      // Calculate the start and end time in ns precision for the following entry.
      start  = ((s_h * 60 * 60 + s_min * 60 + s_sec) * 1'000'000'000ll + s_ns) * s_neg;
      end    = ((e_h * 60 * 60 + e_min * 60 + e_sec) * 1'000'000'000ll + e_ns) * e_neg;

      if (0 > start) {
        mxwarn_tid(m_file_name, m_track_id,
                   fmt::format(FY("Line {0}: Negative timestamp encountered. The entry will be adjusted to start from 00:00:00.000.\n"), line_number));
        end   -= start;
        start  = 0;
        if (0 > end)
          end *= -1;
      }

      // There are files for which start timestamps overlap. Matroska requires
      // blocks to be sorted by their timestamp. mkvmerge does this at the end
      // of this function, but warn the user that the original order is being
      // changed.
      if (!timestamp_warning_printed && (start < previous_start)) {
        mxwarn_tid(m_file_name, m_track_id, fmt::format(FY("Warning in line {0}: The start timestamp is smaller than that of the previous entry. "
                                                           "All entries from this file will be sorted by their start time.\n"), line_number));
        timestamp_warning_printed = true;
      }

      previous_start   = start;
      subtitles        = "";
      state            = STATE_SUBS;
      timestamp_number = subtitle_number;

    } else if (STATE_SUBS == state) {
      if (!subtitles.empty())
        subtitles += "\n";
      subtitles += unstripped_line;

    } else if (Q(s).contains(number_re)) {
      state = STATE_TIME;
      mtx::string::parse_number(s, subtitle_number);

    } else {
      if (!subtitles.empty())
        subtitles += "\n";
      subtitles += unstripped_line;
    }
  }

  if (!subtitles.empty())
    add_maybe(start, end, timestamp_number, subtitles);

  sort();
}

// ------------------------------------------------------------

bool
ssa_parser_c::probe(mm_text_io_c &io) {
  QRegularExpression script_info_re{"^\\s*\\[script\\s+info\\]",   QRegularExpression::CaseInsensitiveOption};
  QRegularExpression styles_re{     "^\\s*\\[V4\\+?\\s+Styles\\]", QRegularExpression::CaseInsensitiveOption};
  QRegularExpression comment_re{    "^\\s*$|^\\s*[!;]",            QRegularExpression::CaseInsensitiveOption};

  try {
    int line_number = 0;
    io.setFilePointer(0);

    std::string line;
    while (io.getline2(line, 1000)) {
      ++line_number;

      // Read at most 100 lines.
      if (100 < line_number)
        return false;

      auto qline = Q(line);
      // Skip comments and empty lines.
      if (qline.contains(comment_re))
        continue;

      // This is the line mkvmerge is looking for: positive match.
      if (qline.contains(script_info_re) || qline.contains(styles_re))
        return true;

      // Neither a wanted line nor an empty one/a comment: negative result.
      return false;
    }
  } catch (...) {
  }

  return false;
}

ssa_parser_c::ssa_parser_c(generic_reader_c &reader,
                           mm_text_io_cptr const &io,
                           const std::string &file_name,
                           int64_t track_id)
  : subtitles_c{file_name, track_id}
  , m_reader(reader)
  , m_io(io)
  , m_is_ass(false)
  , m_attachment_id(0)
{
}

void
ssa_parser_c::parse() {
  QRegularExpression sec_styles_ass_re{"^\\s*\\[V4\\+\\s+Styles\\]", QRegularExpression::CaseInsensitiveOption};
  QRegularExpression sec_styles_re{    "^\\s*\\[V4\\s+Styles\\]",    QRegularExpression::CaseInsensitiveOption};
  QRegularExpression sec_info_re{      "^\\s*\\[Script\\s+Info\\]",  QRegularExpression::CaseInsensitiveOption};
  QRegularExpression sec_events_re{    "^\\s*\\[Events\\]",          QRegularExpression::CaseInsensitiveOption};
  QRegularExpression sec_graphics_re{  "^\\s*\\[Graphics\\]",        QRegularExpression::CaseInsensitiveOption};
  QRegularExpression sec_fonts_re{     "^\\s*\\[Fonts\\]",           QRegularExpression::CaseInsensitiveOption};

  int num                        = 0;
  ssa_section_e section          = SSA_SECTION_NONE;
  ssa_section_e previous_section = SSA_SECTION_NONE;
  std::string name_field         = "Name";

  std::string attachment_name, attachment_data_uu;

  m_io->setFilePointer(0);

  while (!m_io->eof()) {
    std::string line;
    if (!m_io->getline2(line))
      break;

    line               = recode(line);
    auto qline         = Q(line);
    bool add_to_global = true;

    // A normal line. Let's see if this file is ASS and not SSA.
    if (!strcasecmp(line.c_str(), "ScriptType: v4.00+"))
      m_is_ass = true;

    else if (qline.contains(sec_styles_ass_re)) {
      m_is_ass = true;
      section  = SSA_SECTION_V4STYLES;

    } else if (qline.contains(sec_styles_re))
      section = SSA_SECTION_V4STYLES;

    else if (qline.contains(sec_info_re))
      section = SSA_SECTION_INFO;

    else if (qline.contains(sec_events_re))
      section = SSA_SECTION_EVENTS;

    else if (qline.contains(sec_graphics_re)) {
      section       = SSA_SECTION_GRAPHICS;
      add_to_global = false;

    } else if (qline.contains(sec_fonts_re)) {
      section       = SSA_SECTION_FONTS;
      add_to_global = false;

    } else if (SSA_SECTION_EVENTS == section) {
      if (balg::istarts_with(line, "Format: ")) {
        // Analyze the format string.
        m_format = mtx::string::split(&line.c_str()[strlen("Format: ")]);
        mtx::string::strip(m_format);

        // Let's see if "Actor" is used in the format instead of "Name".
        size_t i;
        for (i = 0; m_format.size() > i; ++i)
          if (balg::iequals(m_format[i], "actor")) {
            name_field = "Actor";
            break;
          }

      } else if (balg::istarts_with(line, "Dialogue: ")) {
        if (m_format.empty())
          throw mtx::input::extended_x(Y("ssa_reader: Invalid format. Could not find the \"Format\" line in the \"[Events]\" section."));

        std::string orig_line = line;

        line.erase(0, strlen("Dialogue: ")); // Trim the start.

        // Split the line into fields.
        std::vector<std::string> fields = mtx::string::split(line.c_str(), ",", m_format.size());
        while (fields.size() < m_format.size())
          fields.push_back(""s);

        // Parse the start time.
        auto stime = get_element("Start", fields);
        auto start = parse_time(stime);
        stime      = get_element("End", fields);
        auto end   = parse_time(stime);

        if (   (0     > start)
            || (0     > end)
            || (start > end)) {
          mxwarn_tid(m_file_name, m_track_id, fmt::format(FY("SSA/ASS: The following line will be skipped as one of the timestamps is less than 0, or the end timestamp is less than the start timestamp: {0}\n"), orig_line));
          continue;
        }

        // Specs say that the following fields are to put into the block:
        // ReadOrder, Layer, Style, Name, MarginL, MarginR, MarginV, Effect,
        //   Text

        std::string comma = ",";
        line
          = fmt::to_string(num)                     + comma
          + get_element("Layer", fields)            + comma
          + get_element("Style", fields)            + comma
          + get_element(name_field.c_str(), fields) + comma
          + get_element("MarginL", fields)          + comma
          + get_element("MarginR", fields)          + comma
          + get_element("MarginV", fields)          + comma
          + get_element("Effect", fields)           + comma
          + get_element("Text", fields);

        add(start, end, num, line);
        num++;

        add_to_global = false;
      }

    } else if ((SSA_SECTION_FONTS == section) || (SSA_SECTION_GRAPHICS == section)) {
      if (balg::istarts_with(line, "fontname:")) {
        add_attachment_maybe(attachment_name, attachment_data_uu, section);

        line.erase(0, strlen("fontname:"));
        mtx::string::strip(line, true);
        attachment_name = line;

      } else {
        mtx::string::strip(line, true);
        attachment_data_uu += line;
      }

      add_to_global = false;
    }

    if (add_to_global) {
      m_global += line;
      m_global += "\r\n";
    }

    if (previous_section != section)
      add_attachment_maybe(attachment_name, attachment_data_uu, previous_section);

    previous_section = section;
  }

  sort();
}

std::string
ssa_parser_c::get_element(const char *index,
                          std::vector<std::string> &fields) {
  size_t i;

  for (i = 0; i < m_format.size(); i++)
    if (m_format[i] == index)
      return fields[i];

  return ""s;
}

int64_t
ssa_parser_c::parse_time(std::string &stime) {
  int64_t th, tm, ts, tds;

  int pos = stime.find(':');
  if (0 > pos)
    return -1;

  std::string s = stime.substr(0, pos);
  if (!mtx::string::parse_number(s, th))
    return -1;
  stime.erase(0, pos + 1);

  pos = stime.find(':');
  if (0 > pos)
    return -1;

  s = stime.substr(0, pos);
  if (!mtx::string::parse_number(s, tm))
    return -1;
  stime.erase(0, pos + 1);

  pos = stime.find('.');
  if (0 > pos)
    return -1;

  s = stime.substr(0, pos);
  if (!mtx::string::parse_number(s, ts))
    return -1;
  stime.erase(0, pos + 1);

  if (!mtx::string::parse_number(stime, tds))
    return -1;

  return (tds * 10 + ts * 1000 + tm * 60 * 1000 + th * 60 * 60 * 1000) * 1000000;
}

void
ssa_parser_c::add_attachment_maybe(std::string &name,
                                   std::string &data_uu,
                                   ssa_section_e section) {
  if (name.empty() || data_uu.empty() || ((SSA_SECTION_FONTS != section) && (SSA_SECTION_GRAPHICS != section))) {
    name    = "";
    data_uu = "";
    return;
  }

  ++m_attachment_id;

  if (!m_reader.attachment_requested(m_attachment_id)) {
    name    = "";
    data_uu = "";
    return;
  }

  auto attachment_p = std::make_shared<attachment_t>();
  auto &attachment  = *attachment_p;

  std::string short_name = m_file_name;
  size_t pos             = short_name.rfind('/');

  if (std::string::npos != pos)
    short_name.erase(0, pos + 1);
  pos = short_name.rfind('\\');
  if (std::string::npos != pos)
    short_name.erase(0, pos + 1);

  attachment.ui_id        = m_attachment_id;
  attachment.name         = name;
  attachment.description  = fmt::format(fmt::runtime(SSA_SECTION_FONTS == section ? Y("Imported font from {0}") : Y("Imported picture from {0}")), short_name);
  attachment.to_all_files = true;
  attachment.source_file  = m_file_name;

  size_t data_size        = data_uu.length() % 4;
  data_size               = 3 == data_size ? 2 : 2 == data_size ? 1 : 0;
  data_size              += data_uu.length() / 4 * 3;
  attachment.data         = memory_c::alloc(data_size);
  auto out                = attachment.data->get_buffer();
  auto in                 = reinterpret_cast<uint8_t const *>(data_uu.c_str());

  for (auto end = in + (data_uu.length() / 4) * 4; in < end; in += 4, out += 3)
    decode_chars(in, out, 4);

  decode_chars(in, out, data_uu.length() % 4);

  attachment.mime_type = ::mtx::mime::guess_type_for_data(*attachment.data);
  attachment.mime_type = ::mtx::mime::get_font_mime_type_to_use(attachment.mime_type, g_use_legacy_font_mime_types ? mtx::mime::font_mime_type_type_e::legacy : mtx::mime::font_mime_type_type_e::current);

  add_attachment(attachment_p);

  name    = "";
  data_uu = "";
}

void
ssa_parser_c::decode_chars(uint8_t const *in,
                           uint8_t *out,
                           size_t bytes_in) {
  if (!bytes_in)
    return;

  size_t bytes_out = 4 == bytes_in ? 3 : 3 == bytes_in ? 2 : 1;
  uint32_t value   = 0;

  for (int idx = 0; idx < static_cast<int>(bytes_in); ++idx)
    value |= (static_cast<uint32_t>(in[idx]) - 33) << (6 * (3 - idx));

  for (int idx = 0; idx < static_cast<int>(bytes_out); ++idx)
    out[idx] = (value >> ((2 - idx) * 8)) & 0xff;
}
