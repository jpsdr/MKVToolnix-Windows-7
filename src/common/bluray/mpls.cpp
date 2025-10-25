/*
  mkvmerge -- utility for splicing together matroska files
  from component media subtypes

  Distributed under the GPL v2
  see the file COPYING for details
  or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

  definitions and helper functions for Blu-ray playlist files (MPLS)

  Written by Moritz Bunkus <mo@bunkus.online>.
*/
#include "common/common_pch.h"

#include <vector>

#include <QRegularExpression>

#include "common/bluray/mpls.h"
#include "common/bluray/track_chapter_names.h"
#include "common/debugging.h"
#include "common/hacks.h"
#include "common/list_utils.h"
#include "common/path.h"
#include "common/qt.h"
#include "common/strings/formatting.h"

namespace mtx::bluray::mpls {

static timestamp_c
mpls_time_to_timestamp(uint64_t value) {
  return timestamp_c::ns(value * 1000000ull / 45);
}

void
header_t::dump()
  const {
  mxinfo(fmt::format("  header dump\n"
                     "    type_indicator1 & 2:          {0} / {1}\n"
                     "    playlist / chapter / ext pos: {2} / {3} / {4}\n",
                     type_indicator1, type_indicator2,
                     playlist_pos, chapter_pos, ext_pos));
}

void
playlist_t::dump()
  const {
  mxinfo(fmt::format("  playlist dump\n"
                     "    list_count / sub_count: {0} / {1}\n"
                     "    duration:               {2}\n",
                     list_count, sub_count,
                     duration));

  for (auto &item : items)
    item.dump();

  for (auto const &path : sub_paths)
    path.dump();
}

void
play_item_t::dump()
  const {
  mxinfo(fmt::format("    play item dump\n"
                     "      clip_id / codec_id:      {0} / {1}\n"
                     "      connection_condition:    {2}\n"
                     "      is_multi_angle / stc_id: {3} / {4}\n"
                     "      in_time / out_time:      {5} / {6}\n"
                     "      relative_in_time / end:  {7} / {8}\n",
                     clip_id, codec_id,
                     connection_condition,
                     is_multi_angle, stc_id,
                     in_time, out_time,
                     relative_in_time, relative_in_time + out_time - in_time));

  stn.dump();
}

void
sub_path_t::dump()
  const {
  auto type_name = sub_path_type_e::primary_audio_of_browsable_slideshow       == type ? "primary_audio_of_browsable_slideshow"
                 : sub_path_type_e::interactive_graphics_presentation_menu     == type ? "interactive_graphics_presentation_menu"
                 : sub_path_type_e::text_subtitle_presentation                 == type ? "text_subtitle_presentation"
                 : sub_path_type_e::out_of_mux_synchronous_elementary_streams  == type ? "out_of_mux_synchronous_elementary_streams"
                 : sub_path_type_e::out_of_mux_asynchronous_picture_in_picture == type ? "out_of_mux_asynchronous_picture_in_picture"
                 : sub_path_type_e::in_mux_synchronous_picture_in_picture      == type ? "in_mux_synchronous_picture_in_picture"
                 :                                                                       "reserved";

  mxinfo(fmt::format("    sub path dump\n"
                     "      type / is_repeat:        {0} [{1}] / {2}\n",
                     static_cast<unsigned int>(type), type_name, is_repeat_sub_path));

  for (auto const &item : items)
    item.dump();
}

void
sub_play_item_t::dump()
  const {
  mxinfo(fmt::format("      sub play item dump\n"
                     "        clip_id / codec_id:    {0} / {1}\n"
                     "        conn_con / sync_pi_id: {2} / {3}\n"
                     "        ref_to_stc_id / multi: {4} / {5}\n"
                     "        in_time / out_time:    {6} / {7}\n"
                     "        sync_start_pts:        {8}\n",
                     clpi_file_name       , codec_id,
                     connection_condition, sync_playitem_id,
                     ref_to_stc_id        , is_multi_clip_entries,
                     in_time              , out_time,
                     sync_start_pts_of_playitem));

  for (auto const &clip : clips)
    clip.dump();
}

void
sub_play_item_clip_t::dump()
  const {
  mxinfo(fmt::format("        sub play item clip dump\n"
                     "          clip_id / codec_id:  {0} / {1}\n"
                     "        ref_to_stc_id:         {2}\n",
                     clpi_file_name, codec_id,
                     ref_to_stc_id));
}

void
stn_t::dump()
  const {
  mxinfo(fmt::format("      stn dump\n"
                     "        num_video / num_audio / num_pg / num_ig:    {0} / {1} / {2} / {3}\n"
                     "        num_sec_video / num_sec_audio / num_pip_pg: {4} / {5} / {6}\n",
                     num_video, num_audio, num_pg, num_ig,
                     num_secondary_video, num_secondary_audio, num_pip_pg));

  for (auto &stream : video_streams)
    stream.dump("video");

  for (auto &stream : audio_streams)
    stream.dump("audio");

  for (auto &stream : pg_streams)
    stream.dump("pg");
}

void
stream_t::dump(std::string const &type)
  const {
  auto stream_type_name = stream_type_e::used_by_play_item           == stream_type ? "used_by_play_item"
                        : stream_type_e::used_by_sub_path_type_23456 == stream_type ? "used_by_sub_path_type_23456"
                        : stream_type_e::used_by_sub_path_type_7     == stream_type ? "used_by_sub_path_type_7"
                        :                                                             "reserved";

  auto coding_type_name = stream_coding_type_e::mpeg2_video_primary_secondary     == coding_type ? "mpeg2_video_primary_secondary"
                        : stream_coding_type_e::mpeg4_avc_video_primary_secondary == coding_type ? "mpeg4_avc_video_primary_secondary"
                        : stream_coding_type_e::vc1_video_primary_secondary       == coding_type ? "vc1_video_primary_secondary"
                        : stream_coding_type_e::lpcm_audio_primary                == coding_type ? "lpcm_audio_primary"
                        : stream_coding_type_e::ac3_audio_primary                 == coding_type ? "ac3_audio_primary"
                        : stream_coding_type_e::dts_audio_primary                 == coding_type ? "dts_audio_primary"
                        : stream_coding_type_e::truehd_audio_primary              == coding_type ? "truehd_audio_primary"
                        : stream_coding_type_e::eac3_audio_primary                == coding_type ? "eac3_audio_primary"
                        : stream_coding_type_e::dts_hd_audio_primary              == coding_type ? "dts_hd_audio_primary"
                        : stream_coding_type_e::dts_hd_xll_audio_primary          == coding_type ? "dts_hd_xll_audio_primary"
                        : stream_coding_type_e::eac3_audio_secondary              == coding_type ? "eac3_audio_secondary"
                        : stream_coding_type_e::dts_hd_audio_secondary            == coding_type ? "dts_hd_audio_secondary"
                        : stream_coding_type_e::presentation_graphics_subtitles   == coding_type ? "presentation_graphics_subtitles"
                        : stream_coding_type_e::interactive_graphics_menu         == coding_type ? "interactive_graphics_menu"
                        : stream_coding_type_e::text_subtitles                    == coding_type ? "text_subtitles"
                        :                                                                          "reserved";

  mxinfo(fmt::format("        {0} stream dump\n"
                     "          stream_type:                     {1} [{2}]\n"
                     "          sub_path_id / sub_clip_id / pid: {3} / {4} / {5:04x}\n"
                     "          coding_type:                     {6:02x} [{7}]\n"
                     "          format / rate:                   {8} / {9}\n"
                     "          char_code / language:            {10} / {11}\n",
                     type,
                     static_cast<unsigned int>(stream_type), stream_type_name,
                     sub_path_id, sub_clip_id, pid,
                     static_cast<unsigned int>(coding_type), coding_type_name,
                     format, rate,
                     char_code, language));
}

parser_c::parser_c()
  : m_ok{}
  , m_drop_last_entry_if_at_end{true}
  , m_debug{"mpls"}
  , m_header(header_t())
{
}

void
parser_c::enable_dropping_last_entry_if_at_end(bool enable) {
  m_drop_last_entry_if_at_end = enable;
}

bool
parser_c::parse(mm_io_c &file) {
  try {
    file.setFilePointer(0);
    int64_t file_size = file.get_size();

    if ((4 * 5 > file_size) || (10 * 1024 * 1024 < file_size))
      throw mtx::bluray::mpls::exception(fmt::format("File too small or too big: {0}", file_size));

    auto content = file.read(4 * 5);
    m_bc         = std::make_shared<mtx::bits::reader_c>(content->get_buffer(), 4 * 5);
    parse_header();

    file.setFilePointer(0);

    content = file.read(file_size);
    m_bc    = std::make_shared<mtx::bits::reader_c>(content->get_buffer(), file_size);

    parse_playlist();
    parse_chapters();
    read_chapter_names(file.get_file_name());

    m_bc.reset();

    m_ok = true;

  } catch (mtx::bluray::mpls::exception &ex) {
    mxdebug_if(m_debug, fmt::format("MPLS exception: {0}\n", ex.what()));
  } catch (mtx::mm_io::exception &ex) {
    mxdebug_if(m_debug, fmt::format("I/O exception: {0}\n", ex.what()));
  }

  if (m_debug)
    dump();

  file.setFilePointer(0);

  return m_ok;
}

void
parser_c::parse_header() {
  m_header.type_indicator1 = fourcc_c{static_cast<uint32_t>(m_bc->get_bits(32))};
  m_header.type_indicator2 = fourcc_c{static_cast<uint32_t>(m_bc->get_bits(32))};
  m_header.playlist_pos    = m_bc->get_bits(32);
  m_header.chapter_pos     = m_bc->get_bits(32);
  m_header.ext_pos         = m_bc->get_bits(32);

  if (   (m_header.type_indicator1 != fourcc_c{"MPLS"})
      || !mtx::included_in(m_header.type_indicator2, fourcc_c{"0100"}, fourcc_c{"0200"}, fourcc_c{"0300"}))
    throw exception{fmt::format("Wrong type indicator 1 ({0}) or 2 ({1})", m_header.type_indicator1, m_header.type_indicator2)};
}

void
parser_c::parse_playlist() {
  m_playlist.duration = timestamp_c::ns(0);

  m_bc->set_bit_position(m_header.playlist_pos * 8);
  m_bc->skip_bits(32 + 16);     // playlist length, reserved bytes

  m_playlist.list_count = m_bc->get_bits(16);
  m_playlist.sub_count  = m_bc->get_bits(16);

  m_playlist.items.reserve(m_playlist.list_count);

  for (auto idx = 0u; idx < m_playlist.list_count; ++idx)
    m_playlist.items.push_back(parse_play_item());

  for (auto idx = 0u; idx < m_playlist.sub_count; ++idx)
    m_playlist.sub_paths.push_back(parse_sub_path());
}

play_item_t
parser_c::parse_play_item() {
  auto item     = play_item_t();

  auto length   = m_bc->get_bits(16);
  auto position = m_bc->get_bit_position() / 8u;

  item.clip_id  = read_string(5);
  item.codec_id = read_string(4);

  m_bc->skip_bits(11);          // reserved

  item.is_multi_angle       = m_bc->get_bit();
  item.connection_condition = m_bc->get_bits(4);
  item.stc_id               = m_bc->get_bits(8);
  item.in_time              = mpls_time_to_timestamp(m_bc->get_bits(32));
  item.out_time             = mpls_time_to_timestamp(m_bc->get_bits(32));
  item.relative_in_time     = m_playlist.duration;
  m_playlist.duration      += item.out_time - item.in_time;

  m_bc->skip_bits(12 * 8);      // UO_mask_table, random_access_flag, reserved, still_mode

  if (item.is_multi_angle) {
    unsigned int num_angles = m_bc->get_bits(8);
    m_bc->skip_bits(8);         // reserved, is_differend_audio, is_seamless_angle_change

    if (0 < num_angles)
      m_bc->skip_bits((num_angles - 1) * 10 * 8); // clip_id, clip_codec_id, stc_id
  }

  m_bc->skip_bits(16 + 16);     // STN length, reserved

  item.stn = parse_stn();

  m_bc->set_bit_position((position + length) * 8);

  return item;
}

sub_path_t
parser_c::parse_sub_path() {
  auto path = sub_path_t{};

  path.type               = static_cast<sub_path_type_e>(m_bc->skip_get_bits(32 + 8, 8)); // length, reserved
  path.is_repeat_sub_path = m_bc->skip_get_bits(15, 1);                                   // reserved
  auto num_sub_play_items = m_bc->skip_get_bits(8, 8);                                    // reserved

  for (auto idx = 0u; idx < num_sub_play_items; ++idx)
    path.items.push_back(parse_sub_play_item());

  return path;
}

sub_play_item_t
parser_c::parse_sub_play_item() {
  auto item = sub_play_item_t{};

  m_bc->skip_bits(16);          // length
  item.clpi_file_name             = read_string(5);
  item.codec_id                   = read_string(4);
  item.connection_condition       = m_bc->skip_get_bits(27, 4); // reserved
  item.is_multi_clip_entries      = m_bc->get_bit();
  item.ref_to_stc_id              = m_bc->get_bits(8);
  item.in_time                    = mpls_time_to_timestamp(m_bc->get_bits(32));
  item.out_time                   = mpls_time_to_timestamp(m_bc->get_bits(32));
  item.sync_playitem_id           = m_bc->get_bits(16);
  item.sync_start_pts_of_playitem = mpls_time_to_timestamp(m_bc->get_bits(32));

  if (!item.is_multi_clip_entries)
    return item;

  auto num_clips = m_bc->get_bits(8);
  m_bc->skip_bits(8);           // reserved

  for (auto idx = 1u; idx < num_clips; ++idx)
    item.clips.push_back(parse_sub_play_item_clip());

  return item;
}

sub_play_item_clip_t
parser_c::parse_sub_play_item_clip() {
  auto clip = sub_play_item_clip_t{};

  clip.clpi_file_name = read_string(5);
  clip.codec_id       = read_string(4);
  clip.ref_to_stc_id  = m_bc->get_bits(8);

  return clip;
}

stn_t
parser_c::parse_stn() {
  auto stn = stn_t();

  stn.num_video           = m_bc->get_bits(8);
  stn.num_audio           = m_bc->get_bits(8);
  stn.num_pg              = m_bc->get_bits(8);
  stn.num_ig              = m_bc->get_bits(8);
  stn.num_secondary_audio = m_bc->get_bits(8);
  stn.num_secondary_video = m_bc->get_bits(8);
  stn.num_pip_pg          = m_bc->get_bits(8);

  m_bc->skip_bits(5 * 8);       // reserved

  stn.video_streams.reserve(stn.num_video);
  stn.audio_streams.reserve(stn.num_audio);
  stn.pg_streams.reserve(stn.num_pg);

  for (auto idx = 0u; idx < stn.num_video; ++idx)
    stn.video_streams.push_back(parse_stream());

  for (auto idx = 0u; idx < stn.num_audio; ++idx)
    stn.audio_streams.push_back(parse_stream());

  for (auto idx = 0u; idx < stn.num_pg; ++idx)
    stn.pg_streams.push_back(parse_stream());

  return stn;
}

stream_t
parser_c::parse_stream() {
  auto str        = stream_t();

  auto length     = m_bc->get_bits(8);
  auto position   = m_bc->get_bit_position() / 8u;

  str.stream_type = static_cast<stream_type_e>(m_bc->get_bits(8));

  if (stream_type_e::used_by_play_item == str.stream_type)
    str.pid = m_bc->get_bits(16);

  else if (stream_type_e::used_by_sub_path_type_23456 == str.stream_type) {
    str.sub_path_id = m_bc->get_bits(8);
    str.sub_clip_id = m_bc->get_bits(8);
    str.pid         = m_bc->get_bits(16);

  } else if (stream_type_e::used_by_sub_path_type_7 == str.stream_type) {
    str.sub_path_id = m_bc->get_bits(8);
    str.pid         = m_bc->get_bits(16);

  } else if (m_debug)
    mxdebug(fmt::format("Unknown stream type {0}\n", static_cast<unsigned int>(str.stream_type)));

  m_bc->set_bit_position((length + position) * 8);

  length          = m_bc->get_bits(8);
  position        = m_bc->get_bit_position() / 8u;

  str.coding_type = static_cast<stream_coding_type_e>(m_bc->get_bits(8));

  if (mtx::included_in(str.coding_type, stream_coding_type_e::mpeg2_video_primary_secondary, stream_coding_type_e::mpeg4_avc_video_primary_secondary, stream_coding_type_e::vc1_video_primary_secondary)) {
    str.format = m_bc->get_bits(4);
    str.rate   = m_bc->get_bits(4);

  } else if (mtx::included_in(str.coding_type,
                              stream_coding_type_e::lpcm_audio_primary,   stream_coding_type_e::ac3_audio_primary,    stream_coding_type_e::dts_audio_primary, stream_coding_type_e::truehd_audio_primary,
                              stream_coding_type_e::eac3_audio_primary,   stream_coding_type_e::dts_hd_audio_primary, stream_coding_type_e::dts_hd_xll_audio_primary,
                              stream_coding_type_e::eac3_audio_secondary, stream_coding_type_e::dts_hd_audio_secondary)) {
    str.format   = m_bc->get_bits(4);
    str.rate     = m_bc->get_bits(4);
    str.language = mtx::bcp47::language_c::parse(read_string(3));

  } else if (mtx::included_in(str.coding_type, stream_coding_type_e::presentation_graphics_subtitles, stream_coding_type_e::interactive_graphics_menu))
    str.language = mtx::bcp47::language_c::parse(read_string(3));

  else if (stream_coding_type_e::text_subtitles == str.coding_type) {
    str.char_code = m_bc->get_bits(8);
    str.language  = mtx::bcp47::language_c::parse(read_string(3));

  } else
    mxdebug_if(m_debug, fmt::format("Unrecognized coding type {0:02x}\n", static_cast<unsigned int>(str.coding_type)));

  m_bc->set_bit_position((position + length) * 8);

  return str;
}

void
parser_c::parse_chapters() {
  m_bc->set_bit_position(m_header.chapter_pos * 8);
  m_bc->skip_bits(32);          // unknown
  int num_chapters = m_bc->get_bits(16);

  // 0 unknown
  // 1 type
  // 2, 3 stream_file_index
  // 4, 5, 6, 7 chapter_time
  // 8 - 13 unknown

  for (int idx = 0u; idx < num_chapters; ++idx) {
    m_bc->set_bit_position((m_header.chapter_pos + 4 + 2 + idx * 14) * 8);
    m_bc->skip_bits(8);         // unknown
    if (1 != m_bc->get_bits(8)) // chapter type: entry mark
      continue;

    size_t play_item_idx = m_bc->get_bits(16);
    if (play_item_idx >= m_playlist.items.size())
      continue;

    m_chapters.emplace_back();

    auto &play_item             = m_playlist.items[play_item_idx];
    m_chapters.back().timestamp = mpls_time_to_timestamp(m_bc->get_bits(32)) - play_item.in_time + play_item.relative_in_time;
  }

  if (   !mtx::hacks::is_engaged(mtx::hacks::KEEP_LAST_CHAPTER_IN_MPLS)
      && m_drop_last_entry_if_at_end
      && (0 < num_chapters)
      && (timestamp_c::s(5) >= (m_playlist.duration - m_chapters.back().timestamp)))
    m_chapters.pop_back();

  std::stable_sort(m_chapters.begin(), m_chapters.end(), [](auto const &a, auto const &b) { return a.timestamp < b.timestamp; });
}

std::string
parser_c::read_string(unsigned int length) {
  std::string str(length, ' ');
  m_bc->get_bytes(reinterpret_cast<uint8_t *>(&str[0]), length);

  return str;
}

void
parser_c::read_chapter_names(std::string const &base_file_name) {
  auto matches = QRegularExpression{"(.{5})\\.mpls$"}.match(Q(base_file_name));
  if (!matches.hasMatch())
    return;

  auto all_names = mtx::bluray::track_chapter_names::locate_and_parse_for_title(mtx::fs::to_path(base_file_name), to_utf8(matches.captured(1)));

  for (auto chapter_idx = 0, num_chapters = static_cast<int>(m_chapters.size()); chapter_idx < num_chapters; ++chapter_idx)
    for (auto const &[language, names] : all_names)
      if (chapter_idx < static_cast<int>(names.size()))
        m_chapters[chapter_idx].names.push_back({ mtx::bcp47::language_c::parse(language), names[chapter_idx] });
}

void
parser_c::dump()
  const {
  mxinfo(fmt::format("MPLS class dump\n"
                     "  ok:           {0}\n"
                     "  num_chapters: {1}\n",
                     m_ok, m_chapters.size()));
  for (auto &entry : m_chapters) {
    std::vector<std::string> names;
    for (auto const &name : entry.names)
      names.push_back(fmt::format("{}:{}", name.language, name.name));

    auto names_str = names.empty() ? ""s : fmt::format(" {}", mtx::string::join(names, " "));

    mxinfo(fmt::format("    {0}{1}\n", entry.timestamp, names_str));
  }

  m_header.dump();
  m_playlist.dump();
}

std::string
get_stream_coding_type_description(uint8_t coding_type) {
  switch (coding_type) {
    case static_cast<uint8_t>(stream_coding_type_e::mpeg2_video_primary_secondary):      return "MPEG-2 video primary/secondary"s;
    case static_cast<uint8_t>(stream_coding_type_e::mpeg4_avc_video_primary_secondary):  return "MPEG-4 AVC primary/secondary"s;
    case static_cast<uint8_t>(stream_coding_type_e::mpegh_hevc_video_primary_secondary): return "MPEG-H HEVC primary/secondary"s;
    case static_cast<uint8_t>(stream_coding_type_e::vc1_video_primary_secondary):        return "VC-1 video primary/secondary"s;
    case static_cast<uint8_t>(stream_coding_type_e::lpcm_audio_primary):                 return "LPCM audio primary"s;
    case static_cast<uint8_t>(stream_coding_type_e::ac3_audio_primary):                  return "Dolby Digital (AC-3) audio primary"s;
    case static_cast<uint8_t>(stream_coding_type_e::dts_audio_primary):                  return "DTS audio primary"s;
    case static_cast<uint8_t>(stream_coding_type_e::truehd_audio_primary):               return "Dolby Lossless (TrueHD) audio primary"s;
    case static_cast<uint8_t>(stream_coding_type_e::eac3_audio_primary):                 return "Dolby Digital Plus (E-AC-3) audio primary"s;
    case static_cast<uint8_t>(stream_coding_type_e::dts_hd_audio_primary):               return "DTS HD except XLL audio primary"s;
    case static_cast<uint8_t>(stream_coding_type_e::dts_hd_xll_audio_primary):           return "DTS HD XLL audio primary"s;
    case static_cast<uint8_t>(stream_coding_type_e::eac3_audio_secondary):               return "Dolby Digital Plus (E-AC-3) audio secondary"s;
    case static_cast<uint8_t>(stream_coding_type_e::dts_hd_audio_secondary):             return "DTS HD except XLL audio secondary"s;
    case static_cast<uint8_t>(stream_coding_type_e::presentation_graphics_subtitles):    return "Presentation Graphics subtitles"s;
    case static_cast<uint8_t>(stream_coding_type_e::interactive_graphics_menu):          return "Interactive Graphics menu"s;
    case static_cast<uint8_t>(stream_coding_type_e::text_subtitles):                     return "Text subtitles"s;
  }

  return "unknown"s;
}

std::string
get_video_format_description(uint8_t video_format) {
  switch (video_format) {
    case static_cast<uint8_t>(video_format_e::i480):  return "480i"s;
    case static_cast<uint8_t>(video_format_e::i576):  return "576i"s;
    case static_cast<uint8_t>(video_format_e::p480):  return "480p"s;
    case static_cast<uint8_t>(video_format_e::i1080): return "1080i"s;
    case static_cast<uint8_t>(video_format_e::p720):  return "720p"s;
    case static_cast<uint8_t>(video_format_e::p1080): return "1080p"s;
    case static_cast<uint8_t>(video_format_e::p576):  return "576p"s;
  }

  return "reserved"s;
}

}
