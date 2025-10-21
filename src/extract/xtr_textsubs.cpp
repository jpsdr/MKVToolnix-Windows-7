/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   extracts tracks from Matroska files into other files

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/common_urls.h"
#include "common/codec.h"
#include "common/ebml.h"
#include "common/mm_io_x.h"
#include "common/mm_file_io.h"
#include "common/mm_proxy_io.h"
#include "common/mm_text_io.h"
#include "common/qt.h"
#include "common/strings/editing.h"
#include "common/strings/formatting.h"
#include "common/strings/parsing.h"
#include "extract/xtr_textsubs.h"

xtr_srt_c::xtr_srt_c(const std::string &codec_id,
                     int64_t tid,
                     track_spec_t &tspec)
  : xtr_base_c(codec_id, tid, tspec)
  , m_num_entries(0)
  , m_sub_charset(tspec.sub_charset)
  , m_conv(charset_converter_c::init(tspec.sub_charset))
{
}

void
xtr_srt_c::create_file(xtr_base_c *master,
                       libmatroska::KaxTrackEntry &track) {
  xtr_base_c::create_file(master, track);
  m_out->write_bom(m_sub_charset);
}

void
xtr_srt_c::handle_frame(xtr_frame_t &f) {
  if (!m_entry.m_text.empty()) {
    m_entry.m_duration = std::abs(f.timestamp - m_entry.m_timestamp);
    flush_entry();
  }

  m_entry.m_timestamp = f.timestamp;
  m_entry.m_duration  = f.duration;
  m_entry.m_text      = m_conv->native(f.frame->to_string());
  m_entry.m_text      = to_utf8(Q(m_entry.m_text).replace(QRegularExpression{"\r+"}, {}));

  if (m_entry.m_duration && !m_entry.m_text.empty())
    flush_entry();
}

void
xtr_srt_c::flush_entry() {
  ++m_num_entries;

  auto start =  m_entry.m_timestamp                       / 1'000'000;
  auto end   = (m_entry.m_timestamp + m_entry.m_duration) / 1'000'000;
  auto text  =
    fmt::format("{0}\n"
                "{1:02}:{2:02}:{3:02},{4:03} --> {5:02}:{6:02}:{7:02},{8:03}\n"
                "{9}\n\n",
                m_num_entries,
                start / 1'000 / 60 / 60, (start / 1'000 / 60) % 60, (start / 1'000) % 60, start % 1'000,
                end   / 1'000 / 60 / 60, (end   / 1'000 / 60) % 60, (end   / 1'000) % 60, end   % 1'000,
                m_entry.m_text);

  m_out->puts(text);
  m_out->flush();

  m_entry.m_text.clear();
}

// ------------------------------------------------------------------------

xtr_ssa_c::xtr_ssa_c(const std::string &codec_id,
                     int64_t tid,
                     track_spec_t &tspec)
  : xtr_base_c(codec_id, tid, tspec)
  , m_sub_charset(tspec.sub_charset)
  , m_conv(charset_converter_c::init(tspec.sub_charset))
  , m_warning_printed(false)
  , m_num_fields{}
{
}

void
xtr_ssa_c::create_file(xtr_base_c *master,
                       libmatroska::KaxTrackEntry &track) {
  auto priv = find_child<libmatroska::KaxCodecPrivate>(&track);
  if (!priv)
    mxerror(fmt::format(FY("Track {0} with the CodecID '{1}' is missing the \"codec private\" element and cannot be extracted.\n"), m_tid, m_codec_id));

  xtr_base_c::create_file(master, track);
  m_out->write_bom(m_sub_charset);

  memory_cptr mpriv       = decode_codec_private(priv);

  const uint8_t *pd = mpriv->get_buffer();
  int priv_size           = mpriv->get_size();
  unsigned int bom_len    = 0;
  byte_order_mark_e byte_order_mark = byte_order_mark_e::none;

  // Skip any BOM that might be present.
  mm_text_io_c::detect_byte_order_marker(pd, priv_size, byte_order_mark, bom_len);

  pd                += bom_len;
  priv_size         -= bom_len;

  char *s            = new char[priv_size + 1];
  memcpy(s, pd, priv_size);
  s[priv_size]       = 0;
  std::string sconv  = s;
  delete []s;

  const char *p1;
  if (!(p1 = strstr(sconv.c_str(), "[Events]")) || !strstr(p1, "Format:")) {
    if (m_codec_id == MKV_S_TEXTSSA)
      sconv += "\n[Events]\nFormat: Marked, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text\n";
    else
      sconv += "\n[Events]\nFormat: Layer, Start, End, Style, Actor, MarginL, MarginR, MarginV, Effect, Text\n";

  }

  mtx::string::strip_back(sconv, true);
  sconv += "\n";

  // Keep the Format: line so that the extracted file has the
  // correct field order.
  int pos1 = sconv.find("Format:", sconv.find("[Events]"));
  if (0 > pos1)
    mxerror(fmt::format(FY("Internal bug: tracks.cpp SSA #1. {0}"), BUGMSG));

  int pos2 = sconv.find("\n", pos1);
  if (0 > pos2)
    pos2 = sconv.length();

  std::string format_line = balg::to_lower_copy(sconv.substr(pos1 + 7, pos2 - pos1 - 7));
  if (std::string::npos == format_line.find("text")) {
    if (format_line[format_line.length() - 1] == '\r') {
      format_line.erase(format_line.length() - 1);
      --pos2;
    }

    format_line += ",text";
    sconv.insert(pos2, ", Text");
  }

  m_ssa_format = mtx::string::split(format_line, ",");
  mtx::string::strip(m_ssa_format, true);

  m_num_fields = 1;             // ReadOrder
  for (auto const &field : m_ssa_format)
    if ((field != "start") && (field != "end"))
      ++m_num_fields;

  auto section_after_events_pos = sconv.find("\n[", sconv.find("[Events]"));
  if (std::string::npos != section_after_events_pos) {
    m_priv_post_events = sconv.substr(section_after_events_pos);
    sconv.erase(section_after_events_pos + 1);
  }

  sconv = m_conv->native(sconv);
  m_out->puts(sconv);
}

void
xtr_ssa_c::handle_frame(xtr_frame_t &f) {
  if (0 > f.duration) {
    mxwarn(fmt::format(FY("Subtitle track {0} is missing some duration elements. "
                          "Please check the resulting SSA/ASS file for entries that have the same start and end time.\n"),
                       m_tid));
    m_warning_printed = true;
  }

  int64_t start =         f.timestamp / 1000000;
  int64_t end   = start + f.duration / 1000000;

  auto af_s     = memory_c::alloc(f.frame->get_size() + 1);
  char *s       = reinterpret_cast<char *>(af_s->get_buffer());
  memcpy(s, f.frame->get_buffer(), f.frame->get_size());
  s[f.frame->get_size()] = 0;

  // Split the line into the fields.
  auto fields = mtx::string::split(s, ",", m_num_fields);

  while (m_num_fields != fields.size())
    fields.emplace_back("");

  // Convert the ReadOrder entry so that we can re-order the entries later.
  int num;
  if (!mtx::string::parse_number(fields[0], num)) {
    mxwarn(fmt::format(FY("Invalid format for a SSA line ('{0}') at timestamp {1}: The first field is not an integer. This entry will be skipped.\n"),
                       s, mtx::string::format_timestamp(f.timestamp * 1000000, 3)));
    return;
  }

  // Reconstruct the 'original' line. It'll look like this for SSA:
  //   Marked, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text
  // and for ASS:
  //   Layer, Start, End, Style, Actor, MarginL, MarginR, MarginV, Effect, Text

  // Problem is that the CodecPrivate may contain a Format: line
  // that defines a different layout. So let's account for that.

  std::string line = "Dialogue: ";
  auto field_idx   = 1u;

  for (auto i = 0u; i < m_ssa_format.size(); i++) {
    auto format = m_ssa_format[i];

    if (0 < i)
      line += ",";

    if (format == "marked") {
      line += fmt::format("Marked={0}", (field_idx < m_num_fields) && !fields[field_idx].empty() ? fields[field_idx] : "0");
      ++field_idx;

    } else if (format == "start")
      line += fmt::format("{0}:{1:02}:{2:02}.{3:02}", start / 1'000 / 60 / 60, (start / 1'000 / 60) % 60, (start / 1'000) % 60, (start % 1'000) / 10);

    else if (format == "end")
      line += fmt::format("{0}:{1:02}:{2:02}.{3:02}", end   / 1'000 / 60 / 60, (end   / 1'000 / 60) % 60, (end   / 1'000) % 60, (end   % 1'000) / 10);

    else if (field_idx < m_num_fields)
      line += fields[field_idx++];
  }

  // Do the charset conversion.
  line  = m_conv->native(line);
  line += "\n";

  // Now store that entry.
  m_lines.push_back(ssa_line_c(line, num));
}

void
xtr_ssa_c::finish_file() {
  size_t i;

  // Sort the SSA lines according to their ReadOrder number and
  // write them.
  std::sort(m_lines.begin(), m_lines.end());
  for (i = 0; i < m_lines.size(); i++)
    m_out->puts(m_lines[i].m_line.c_str());

  if (!m_priv_post_events.empty())
    m_out->puts(m_conv->native(m_priv_post_events));
}

// ------------------------------------------------------------------------

xtr_usf_c::xtr_usf_c(const std::string &codec_id,
                     int64_t tid,
                     track_spec_t &tspec)
  : xtr_base_c(codec_id, tid, tspec)
  , m_sub_charset(tspec.sub_charset)
{
  if (m_sub_charset.empty())
    m_sub_charset = "UTF-8";

  m_simplified_sub_charset = to_utf8(Q(m_sub_charset).toLower().replace(QRegularExpression{"[^a-z0-9]+"}, {}));
}

void
xtr_usf_c::create_file(xtr_base_c *master,
                       libmatroska::KaxTrackEntry &track) {
  auto priv = find_child<libmatroska::KaxCodecPrivate>(&track);
  if (!priv)
    mxerror(fmt::format(FY("Track {0} with the CodecID '{1}' is missing the \"codec private\" element and cannot be extracted.\n"), m_tid, m_codec_id));

  init_content_decoder(track);

  memory_cptr new_priv = decode_codec_private(priv);
  m_codec_private.append((const char *)new_priv->get_buffer(), new_priv->get_size());

  m_language = get_track_language(track);

  if (master) {
    xtr_usf_c *usf_master = dynamic_cast<xtr_usf_c *>(master);
    if (!usf_master)
      mxerror(fmt::format(FY("Cannot write track {0} with the CodecID '{1}' to the file '{2}' because "
                             "track {3} with the CodecID '{4}' is already being written to the same file.\n"),
                          m_tid, m_codec_id, m_file_name, master->m_tid, master->m_codec_id));

    if (m_codec_private != usf_master->m_codec_private)
      mxerror(fmt::format(FY("Cannot write track {0} with the CodecID '{1}' to the file '{2}' because track {3} with the CodecID '{4}' is already "
                             "being written to the same file, and their CodecPrivate data (the USF styles etc.) do not match.\n"),
                          m_tid, m_codec_id, m_file_name, master->m_tid, master->m_codec_id));

    m_doc    = usf_master->m_doc;
    m_master = usf_master;

  } else {
    try {
      m_out = mm_file_io_c::open(m_file_name, libebml::MODE_CREATE);
      m_doc = std::make_shared<pugi::xml_document>();

      std::stringstream codec_private{m_codec_private};
      auto result = m_doc->load(codec_private, pugi::parse_default | pugi::parse_declaration | pugi::parse_doctype | pugi::parse_pi | pugi::parse_comments);
      if (!result)
        throw mtx::xml::xml_parser_x{result};

      pugi::xml_node doctype_node, declaration_node, stylesheet_node;
      for (auto child : *m_doc)
        if (child.type() == pugi::node_declaration)
          declaration_node = child;

        else if (child.type() == pugi::node_doctype)
          doctype_node = child;

        else if ((child.type() == pugi::node_pi) && (std::string{child.name()} == "xml-stylesheet"))
          stylesheet_node = child;

      if (!declaration_node)
        declaration_node = m_doc->prepend_child(pugi::node_declaration);

      if (!doctype_node)
        doctype_node = m_doc->insert_child_after(pugi::node_doctype, declaration_node);

      if (!stylesheet_node)
        stylesheet_node = m_doc->insert_child_after(pugi::node_pi, declaration_node);

      if (!balg::starts_with(m_simplified_sub_charset, "utf"))
        declaration_node.append_attribute("encoding").set_value(m_sub_charset.c_str());
      doctype_node.set_value("USFSubtitles SYSTEM \"USFV100.dtd\"");
      stylesheet_node.set_name("xml-stylesheet");
      stylesheet_node.append_attribute("type").set_value("text/xsl");
      stylesheet_node.append_attribute("href").set_value("USFV100.xsl");

    } catch (mtx::mm_io::exception &ex) {
      mxerror(fmt::format(FY("Failed to create the file '{0}': {1}\n"), m_file_name, ex));

    } catch (mtx::xml::exception &ex) {
      mxerror(fmt::format(FY("Failed to parse the USF codec private data for track {0}: {1}\n"), m_tid, ex.what()));
    }
  }
}

void
xtr_usf_c::handle_frame(xtr_frame_t &f) {
  usf_entry_t entry("", f.timestamp, f.timestamp + f.duration);
  entry.m_text.append((const char *)f.frame->get_buffer(), f.frame->get_size());
  m_entries.push_back(entry);
}

void
xtr_usf_c::finish_track() {
  auto subtitles = m_doc->document_element().append_child("subtitles");
  subtitles.append_child("language").append_attribute("code").set_value(m_language.get_closest_iso639_2_alpha_3_code().c_str());

  for (auto &entry : m_entries) {
    std::string text = "<subtitle>"s + entry.m_text + "</subtitle>";
    mtx::string::strip(text, true);

    std::stringstream text_in(text);
    pugi::xml_document subtitle_doc;
    if (!subtitle_doc.load(text_in, pugi::parse_default | pugi::parse_declaration | pugi::parse_doctype | pugi::parse_pi | pugi::parse_comments)) {
      mxwarn(fmt::format(FY("Track {0}: An USF subtitle entry starting at timestamp {1} is not well-formed XML and will be skipped.\n"), m_tid, mtx::string::format_timestamp(entry.m_start, 3)));
      continue;
    }

    auto subtitle = subtitles.append_child("subtitle");
    subtitle.append_attribute("start").set_value(mtx::string::format_timestamp(entry.m_start, 3).c_str());
    subtitle.append_attribute("stop"). set_value(mtx::string::format_timestamp(entry.m_end,   3).c_str());

    for (auto child : subtitle_doc.document_element())
      subtitle.append_copy(child);
  }
}

void
xtr_usf_c::finish_file() {
  if (m_master)
    return;

  std::stringstream out;
  m_doc->save(out, "  ");

  m_out->set_string_output_converter(charset_converter_c::init(m_sub_charset));
  m_out->write_bom(m_sub_charset);
  m_out->puts(out.str());
}
