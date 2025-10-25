/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Conversion from teletext to SRT subtitles

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/list_utils.h"
#include "common/qt.h"
#include "common/strings/formatting.h"
#include "input/teletext_to_srt_packet_converter.h"
#include "merge/generic_packetizer.h"

namespace {
auto s_merge_allowed_within = timestamp_c::ms(40);

std::string
displayable_packet_content(memory_c const &mem) {
  return to_utf8(Q(mem.to_string()).replace(QRegularExpression{"\\n+"}, "\\n"));
}
}

std::vector<teletext_to_srt_packet_converter_c::char_map_t> teletext_to_srt_packet_converter_c::ms_char_maps;

static uint8_t invtab[256] = {
  0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
  0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
  0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
  0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
  0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
  0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
  0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
  0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
  0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
  0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
  0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
  0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
  0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
  0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
  0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
  0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
  0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
  0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
  0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
  0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
  0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
  0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
  0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
  0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
  0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
  0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
  0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
  0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
  0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
  0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
  0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
  0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
};

static uint8_t unhamtab[256] = {
  0x01, 0xff, 0x81, 0x01, 0xff, 0x00, 0x01, 0xff,
  0xff, 0x02, 0x01, 0xff, 0x0a, 0xff, 0xff, 0x07,
  0xff, 0x00, 0x01, 0xff, 0x00, 0x80, 0xff, 0x00,
  0x06, 0xff, 0xff, 0x0b, 0xff, 0x00, 0x03, 0xff,
  0xff, 0x0c, 0x01, 0xff, 0x04, 0xff, 0xff, 0x07,
  0x06, 0xff, 0xff, 0x07, 0xff, 0x07, 0x07, 0x87,
  0x06, 0xff, 0xff, 0x05, 0xff, 0x00, 0x0d, 0xff,
  0x86, 0x06, 0x06, 0xff, 0x06, 0xff, 0xff, 0x07,
  0xff, 0x02, 0x01, 0xff, 0x04, 0xff, 0xff, 0x09,
  0x02, 0x82, 0xff, 0x02, 0xff, 0x02, 0x03, 0xff,
  0x08, 0xff, 0xff, 0x05, 0xff, 0x00, 0x03, 0xff,
  0xff, 0x02, 0x03, 0xff, 0x03, 0xff, 0x83, 0x03,
  0x04, 0xff, 0xff, 0x05, 0x84, 0x04, 0x04, 0xff,
  0xff, 0x02, 0x0f, 0xff, 0x04, 0xff, 0xff, 0x07,
  0xff, 0x05, 0x05, 0x85, 0x04, 0xff, 0xff, 0x05,
  0x06, 0xff, 0xff, 0x05, 0xff, 0x0e, 0x03, 0xff,
  0xff, 0x0c, 0x01, 0xff, 0x0a, 0xff, 0xff, 0x09,
  0x0a, 0xff, 0xff, 0x0b, 0x8a, 0x0a, 0x0a, 0xff,
  0x08, 0xff, 0xff, 0x0b, 0xff, 0x00, 0x0d, 0xff,
  0xff, 0x0b, 0x0b, 0x8b, 0x0a, 0xff, 0xff, 0x0b,
  0x0c, 0x8c, 0xff, 0x0c, 0xff, 0x0c, 0x0d, 0xff,
  0xff, 0x0c, 0x0f, 0xff, 0x0a, 0xff, 0xff, 0x07,
  0xff, 0x0c, 0x0d, 0xff, 0x0d, 0xff, 0x8d, 0x0d,
  0x06, 0xff, 0xff, 0x0b, 0xff, 0x0e, 0x0d, 0xff,
  0x08, 0xff, 0xff, 0x09, 0xff, 0x09, 0x09, 0x89,
  0xff, 0x02, 0x0f, 0xff, 0x0a, 0xff, 0xff, 0x09,
  0x88, 0x08, 0x08, 0xff, 0x08, 0xff, 0xff, 0x09,
  0x08, 0xff, 0xff, 0x0b, 0xff, 0x0e, 0x03, 0xff,
  0xff, 0x0c, 0x0f, 0xff, 0x04, 0xff, 0xff, 0x09,
  0x0f, 0xff, 0x8f, 0x0f, 0xff, 0x0e, 0x0f, 0xff,
  0x08, 0xff, 0xff, 0x05, 0xff, 0x0e, 0x0d, 0xff,
  0xff, 0x0e, 0x0f, 0xff, 0x0e, 0x8e, 0xff, 0x0e,
};

// Subtitle conversation color support
//
// ETSI EN 300 706 V1.2.1 (2003-04)
// Enhanced Teletext specification
//
// Table 29: Function of Column Address triplets
// 00000 Foreground Colour
// When data field bits D6 and D5 are both set to '0', bits D4 - D0 define the foreground
// colour. Bits D4 and D3 select a CLUT in the Colour Map of table 30, and bits D2 - D0
// select an entry from that CLUT. All other data field values are reserved.
// The effect of this attribute persists to the end of a display row unless
//
// Table 30: Colour Map
// CLUT Entry Colour
//      0     Black
//      1     Red
//      2     Green
// 0    3     Yellow
//      4     Blue
//      5     Magenta
//      6     Cyan
//      7     White
//
// Only using CLUT 0, the rest is ignored
//
// Implemented in decode_color_text() and decode_color_text_end_of_line()
// Implementation used from https://github.com/CCExtractor/ccextractor/blob/master/src/lib_ccx/telxcc.c

static const char *TTXT_COLOURS[8] = {
    // black,   red,       green,     yellow,    blue,      magenta,   cyan,      white
    "#000000", "#ff0000", "#00ff00", "#ffff00", "#0000ff", "#ff00ff", "#00ffff", "#ffffff"};

bool font_tag_opened = false;

void
teletext_to_srt_packet_converter_c::ttx_page_data_t::reset() {
  page          = -1;
  subpage       = -1;
  flags         =  0;
  national_set  =  0;
  erase_flag    = false;
  subtitle_flag = false;

  page_buffer.clear();
  page_buffer.reserve(TTX_PAGE_TEXT_ROW_SIZE + 1);
  for (auto idx = 0; idx <= TTX_PAGE_TEXT_ROW_SIZE; ++idx)
    page_buffer.emplace_back();
}

teletext_to_srt_packet_converter_c::teletext_to_srt_packet_converter_c()
  : packet_converter_c{nullptr}
  , m_in_size{}
  , m_pos{}
  , m_data_length{}
  , m_buf{}
  , m_page_re1{" *\\n[ \\n]+"}
  , m_page_re2{" +"}
  , m_page_re3{"^[ \\n]+|[ \\n]+$"}
  , m_debug{           "teletext_to_srt_all|teletext_to_srt"}
  , m_debug_packet{    "teletext_to_srt_all|teletext_to_srt_packet"}
  , m_debug_conversion{"teletext_to_srt_all|teletext_to_srt_conversion"}
{
  setup_character_maps();
}

void
teletext_to_srt_packet_converter_c::override_encoding(int page,
                                                      std::string const &iso639_alpha_3_code) {
  auto itr = m_track_data.find(page);
  if (itr == m_track_data.end())
    return;

  if (mtx::included_in(iso639_alpha_3_code, "fre"))
    itr->second->m_forced_char_map_idx = 1;

  else if (mtx::included_in(iso639_alpha_3_code, "swe", "fin", "hun"))
    itr->second->m_forced_char_map_idx = 2;

  else if (mtx::included_in(iso639_alpha_3_code, "cze", "slo"))
    itr->second->m_forced_char_map_idx = 3;

  else if (mtx::included_in(iso639_alpha_3_code, "ger"))
    itr->second->m_forced_char_map_idx = 4;

  else if (mtx::included_in(iso639_alpha_3_code, "por", "spa"))
    itr->second->m_forced_char_map_idx = 5;

  else if (mtx::included_in(iso639_alpha_3_code, "ita"))
    itr->second->m_forced_char_map_idx = 6;

  else if (mtx::included_in(iso639_alpha_3_code, "rum"))
    itr->second->m_forced_char_map_idx = 7;

  else if (mtx::included_in(iso639_alpha_3_code, "lav", "lit"))
    itr->second->m_forced_char_map_idx = 8;

  else if (mtx::included_in(iso639_alpha_3_code, "pol"))
    itr->second->m_forced_char_map_idx = 9;

  else if (mtx::included_in(iso639_alpha_3_code, "srp", "hrv", "slv"))
    itr->second->m_forced_char_map_idx = 10;

  else if (mtx::included_in(iso639_alpha_3_code, "est"))
    itr->second->m_forced_char_map_idx = 11;

  else if (mtx::included_in(iso639_alpha_3_code, "tur"))
    itr->second->m_forced_char_map_idx = 12;

  mxdebug_if(m_debug, fmt::format("Overriding encoding for ISO 639-2 code {0}; result: {1}\n", iso639_alpha_3_code, itr->second->m_forced_char_map_idx ? *itr->second->m_forced_char_map_idx : -1));
}

void
teletext_to_srt_packet_converter_c::demux_page(std::optional<int> page,
                                               generic_packetizer_c *packetizer) {
  if (!page)
    return;

  m_track_data.emplace(*page, std::make_shared<track_data_t>(packetizer));
  m_track_data[*page]->m_page_data.page = *page;
}

void
teletext_to_srt_packet_converter_c::setup_character_maps() {
  if (ms_char_maps.size())
    return;

  // english ,000
  ms_char_maps.push_back({ { 0x23, "£" }, { 0x24, "$" }, { 0x40, "@" }, { 0x5b, "«" }, { 0x5c, "½" }, { 0x5d, "»" }, { 0x5e, "^" }, { 0x5f, "#" }, { 0x60, "-" }, { 0x7b, "¼" }, { 0x7c, "¦" }, { 0x7d, "¾" }, { 0x7e, "÷" } });
  // french  ,001
  ms_char_maps.push_back({ { 0x23, "é" }, { 0x24, "ï" }, { 0x40, "à" }, { 0x5b, "ë" }, { 0x5c, "ê" }, { 0x5d, "ù" }, { 0x5e, "î" }, { 0x5f, "#" }, { 0x60, "è" }, { 0x7b, "â" }, { 0x7c, "ô" }, { 0x7d, "û" }, { 0x7e, "ç" } });
  // swedish,finnish,hungarian ,010
  ms_char_maps.push_back({ { 0x23, "#" }, { 0x24, "¤" }, { 0x40, "É" }, { 0x5b, "Ä" }, { 0x5c, "Ö" }, { 0x5d, "Å" }, { 0x5e, "Ü" }, { 0x5f, "_" }, { 0x60, "é" }, { 0x7b, "ä" }, { 0x7c, "ö" }, { 0x7d, "å" }, { 0x7e, "ü" } });
  // czech,slovak  ,011
  ms_char_maps.push_back({ { 0x23, "#" }, { 0x24, "ů" }, { 0x40, "č" }, { 0x5b, "ť" }, { 0x5c, "ž" }, { 0x5d, "ý" }, { 0x5e, "í" }, { 0x5f, "ř" }, { 0x60, "é" }, { 0x7b, "á" }, { 0x7c, "ě" }, { 0x7d, "ú" }, { 0x7e, "š" } });
  // german ,100
  ms_char_maps.push_back({ { 0x23, "#" }, { 0x24, "$" }, { 0x40, "§" }, { 0x5b, "Ä" }, { 0x5c, "Ö" }, { 0x5d, "Ü" }, { 0x5e, "^" }, { 0x5f, "_" }, { 0x60, "°" }, { 0x7b, "ä" }, { 0x7c, "ö" }, { 0x7d, "ü" }, { 0x7e, "ß" } });
  // portuguese,spanish ,101
  ms_char_maps.push_back({ { 0x23, "ç" }, { 0x24, "$" }, { 0x40, "¡" }, { 0x5b, "á" }, { 0x5c, "é" }, { 0x5d, "í" }, { 0x5e, "ó" }, { 0x5f, "ú" }, { 0x60, "¿" }, { 0x7b, "ü" }, { 0x7c, "ñ" }, { 0x7d, "è" }, { 0x7e, "à" } });
  // italian  ,110
  ms_char_maps.push_back({ { 0x23, "£" }, { 0x24, "$" }, { 0x40, "é" }, { 0x5b, "°" }, { 0x5c, "ç" }, { 0x5d, "»" }, { 0x5e, "^" }, { 0x5f, "#" }, { 0x60, "ù" }, { 0x7b, "à" }, { 0x7c, "ò" }, { 0x7d, "è" }, { 0x7e, "ì" } });
  // rumanian ,111
  ms_char_maps.push_back({ { 0x23, "#" }, { 0x24, "¤" }, { 0x40, "Ţ" }, { 0x5b, "Â" }, { 0x5c, "Ş" }, { 0x5d, "Ă" }, { 0x5e, "Î" }, { 0x5f, "ı" }, { 0x60, "ţ" }, { 0x7b, "â" }, { 0x7c, "ş" }, { 0x7d, "ă" }, { 0x7e, "î" } });
  // lettish,lithuanian ,1000
  ms_char_maps.push_back({ { 0x23, "#" }, { 0x24, "$" }, { 0x40, "Š" }, { 0x5b, "ė" }, { 0x5c, "ę" }, { 0x5d, "Ž" }, { 0x5e, "č" }, { 0x5f, "ū" }, { 0x60, "š" }, { 0x7b, "ą" }, { 0x7c, "ų" }, { 0x7d, "ž" }, { 0x7e, "į" } });
  // polish,  1001
  ms_char_maps.push_back({ { 0x23, "#" }, { 0x24, "ń" }, { 0x40, "ą" }, { 0x5b, "Z" }, { 0x5c, "Ś" }, { 0x5d, "Ł" }, { 0x5e, "ć" }, { 0x5f, "ó" }, { 0x60, "ę" }, { 0x7b, "ż" }, { 0x7c, "ś" }, { 0x7d, "ł" }, { 0x7e, "ź" } });
  // serbian,croatian,slovenian, 1010
  ms_char_maps.push_back({ { 0x23, "#" }, { 0x24, "Ë" }, { 0x40, "Č" }, { 0x5b, "Ć" }, { 0x5c, "Ž" }, { 0x5d, "Đ" }, { 0x5e, "Š" }, { 0x5f, "ë" }, { 0x60, "č" }, { 0x7b, "ć" }, { 0x7c, "ž" }, { 0x7d, "đ" }, { 0x7e, "š" } });
  // estonian  ,1011
  ms_char_maps.push_back({ { 0x23, "#" }, { 0x24, "õ" }, { 0x40, "Š" }, { 0x5b, "Ä" }, { 0x5c, "Ö" }, { 0x5d, "ž" }, { 0x5e, "Ü" }, { 0x5f, "Õ" }, { 0x60, "š" }, { 0x7b, "ä" }, { 0x7c, "ö" }, { 0x7d, "ž" }, { 0x7e, "ü" } });
  // turkish  ,1100
  ms_char_maps.push_back({ { 0x23, "T" }, { 0x24, "ğ" }, { 0x40, "İ" }, { 0x5b, "Ş" }, { 0x5c, "Ö" }, { 0x5d, "Ç" }, { 0x5e, "Ü" }, { 0x5f, "Ğ" }, { 0x60, "ı" }, { 0x7b, "ş" }, { 0x7c, "ö" }, { 0x7d, "ç" }, { 0x7e, "ü" } });
}

void
teletext_to_srt_packet_converter_c::bit_reverse(uint8_t *buffer,
                                                size_t length) {
  for (int idx = 0; idx < static_cast<int>(length); ++idx)
    buffer[idx] = invtab[buffer[idx]];
}

void
teletext_to_srt_packet_converter_c::unham(uint8_t const *in,
                                          uint8_t *out,
                                          size_t hambytes) {
  for (int idx = 0; idx < static_cast<int>(hambytes / 2); ++idx, ++out, in += 2)
    *out = (unhamtab[*in] & 0x0f) | ((unhamtab[*(in + 1)] & 0x0f) << 4);
}


int
teletext_to_srt_packet_converter_c::ttx_to_page(int ttx) {
  int retval = 0;

  for (int idx = 0; idx < 4; ++idx) {
    retval *= 10;
    retval += (ttx & 0xf000) >> 12;
    ttx   <<= 4;
  }

  return retval;
}

void
teletext_to_srt_packet_converter_c::remove_parity(uint8_t *buffer,
                                                  size_t length) {
  for (int idx = 0; idx < static_cast<int>(length); ++idx)
    buffer[idx] &= 0x7f;
}

std::string
teletext_to_srt_packet_converter_c::decode_color_text(uint8_t c) {
  if (c > 0x07)
    return " "s;

  auto font_str = maybe_close_color_font_tag();

  // black is considered as white
  // <font/> tags are only written when needed
  font_str       += fmt::format(" <font color=\"{0}\">", TTXT_COLOURS[c]);
  font_tag_opened = true;

  return font_str;
}

std::string
teletext_to_srt_packet_converter_c::maybe_close_color_font_tag() {
  std::string font_str = "";

  if (font_tag_opened) {
    font_str += "</font>";
    font_tag_opened = false;
  }

  return font_str;
}


bool
teletext_to_srt_packet_converter_c::decode_line(uint8_t const *buffer,
                                                unsigned int row_number) {
  if (!m_current_track)
    return false;

  auto &page_data = m_current_track->m_page_data;
  auto &recoded   = page_data.page_buffer[row_number - 1];
  auto prior      = recoded;

  if (!m_current_track->m_forced_char_map_idx && (page_data.national_set >= ms_char_maps.size())) {
    recoded = std::string{reinterpret_cast<char const *>(buffer), TTX_PAGE_TEXT_COLUMN_SIZE};
    return recoded != prior;
  }

  auto char_map_idx = m_current_track->m_forced_char_map_idx ? *m_current_track->m_forced_char_map_idx : page_data.national_set;
  auto &char_map    = ms_char_maps[char_map_idx];

  recoded.clear();

  for (auto idx = 0u; idx < TTX_PAGE_TEXT_COLUMN_SIZE; ++idx) {
    auto c      = buffer[idx];
    auto mapped = char_map[static_cast<int>(c)];

    mxdebug_if(m_debug_packet, fmt::format("  txt char {0} ({1})\n", c, static_cast<int>(c)));

    recoded += mapped  ? std::string{mapped}
             : c < ' ' ? decode_color_text(c)
             :           std::string{static_cast<char>(c)};

  }

  recoded += maybe_close_color_font_tag();

  auto to_clean = Q(recoded);

  static std::optional<QRegularExpression> s_re_spaces_start_end, s_re_spaces_before_closing_tag, s_re_spaces_after_opening_tag, s_re_no_content;

  if (!s_re_spaces_start_end) {
    s_re_spaces_before_closing_tag = QRegularExpression{Q("[[:space:]]+</font>[[:space:]]*")};
    s_re_spaces_after_opening_tag  = QRegularExpression{Q("[[:space:]]*(<font color=[^>]+>)[[:space:]]+")};
    s_re_spaces_start_end          = QRegularExpression{Q("^[[:space:]]+|[[:space:]]+$")};
    s_re_no_content                = QRegularExpression{Q("<font color=[^>]+>[[:space:]]*</font>")};
  }

  to_clean
    .replace(*s_re_spaces_before_closing_tag, Q("</font> "))
    .replace(*s_re_spaces_after_opening_tag,  Q(" \\1"))
    .replace(*s_re_no_content,                {})
    .replace(*s_re_spaces_start_end,          {});

  recoded = to_utf8(to_clean);

  return recoded != prior;
}

void
teletext_to_srt_packet_converter_c::process_single_row(unsigned int row_number) {
  if (!m_current_track)
    return;

  remove_parity(&m_buf[m_pos + 6], m_data_length + 2 - 6);
  if (decode_line(&m_buf[m_pos + 6], row_number)) {
    m_current_track->m_page_changed = true;
    mxdebug_if(m_debug_packet, fmt::format("  new content of row {0}: {1}\n", row_number, m_current_track->m_page_data.page_buffer[row_number - 1]));
  }
}

void
teletext_to_srt_packet_converter_c::decode_page_data(uint8_t ttx_header_magazine) {
  uint8_t ttx_packet_0_header[4];
  unham(&m_buf[m_pos + 6], ttx_packet_0_header, 8);

  auto page     = ttx_packet_0_header[0] == 0xff ? -1 : ttx_to_page(ttx_packet_0_header[0]) + 100 * ttx_header_magazine;
  auto data_itr = m_track_data.find(page);

  mxdebug_if(m_debug, fmt::format("  decode_page_data: ttx_header_magazine {0} ttx_packet_0_header[0] {1} page {2} have_page {3}\n", ttx_header_magazine, static_cast<unsigned>(ttx_packet_0_header[0]), page, data_itr != m_track_data.end()));

  if (   (page > (0x99 + 100 * ttx_header_magazine))
      || (page < 100)) {
    m_current_track = nullptr;
    return;
  }

  if ((data_itr == m_track_data.end()) && m_parse_for_probing) {
    mxdebug_if(m_debug, fmt::format("  decode_page_data: parse for probing; adding new page {0}\n", page));
    demux_page(page, nullptr);

    data_itr = m_track_data.find(page);
  }

  if (data_itr == m_track_data.end()) {
    m_current_track = nullptr;
    return;
  }

  m_current_track                   = data_itr->second.get();
  m_current_track->m_page_timestamp = m_current_packet_timestamp;
  m_current_track->m_magazine       = ttx_header_magazine;

  auto &page_data                   = m_current_track->m_page_data;

  page_data.subpage                 = ((ttx_packet_0_header[2] << 8) | (ttx_packet_0_header[1])) & 0x3f7f;
  page_data.subpage                 = ttx_to_page(page_data.subpage);

  page_data.flags                   = ((ttx_packet_0_header[1] >> 7) & 0x01)  // C4 Erase Page flag
                                    | ((ttx_packet_0_header[2] >> 5) & 0x06)  // C5 Newsflash + C6 Subtitle flags
                                    |  (ttx_packet_0_header[3] << 3);         // C7-C14: Suppress Header, Update Indicator, Interrupted Sequence
                                                                              // Inhibit Display, Magazine Serial, National Option Character Subset (3 bits)

  page_data.erase_flag              = page_data.flags & 0x01;
  page_data.national_set            = page_data.flags >> 8;
  page_data.subtitle_flag           = (page_data.flags >> 2) & 0x01;

  auto const page_content           = page_to_string();

  mxdebug_if(m_debug,
             fmt::format("  ttx page {0} at {5} subpage {1} erase {2} national set {3} subtitle {4} flags {6:02x} queued timestamp {7} queued content size {8}\n",
                         page_data.page, page_data.subpage, page_data.erase_flag, page_data.national_set, page_data.subtitle_flag, mtx::string::format_timestamp(m_current_packet_timestamp), static_cast<unsigned int>(page_data.flags),
                         m_current_track->m_queued_timestamp, page_content.size()));

  queue_page_content(page_content);

  if (!page_data.erase_flag)
    return;

  for (auto &line : page_data.page_buffer)
    line.clear();

  m_current_track->m_page_changed = page_to_string() != page_content;
}

void
teletext_to_srt_packet_converter_c::queue_page_content(std::string const &content) {
  if (!m_current_track->m_queued_timestamp.valid() && m_current_track->m_page_data.erase_flag)
    deliver_queued_packet();

  if (content.empty() || !m_current_track->m_queued_timestamp.valid()) {
    m_current_track->m_queued_timestamp.reset();
    return;
  }

  auto duration   = (m_current_track->m_page_timestamp - m_current_track->m_queued_timestamp).abs();
  auto new_packet = std::make_shared<packet_t>(memory_c::clone(content), m_current_track->m_queued_timestamp.to_ns(), duration.to_ns());

  queue_packet(new_packet);

  m_current_track->m_queued_timestamp.reset();

  if (m_current_track->m_page_data.subtitle_flag)
    m_current_track->m_subtitle_page_found = true;
}

void
teletext_to_srt_packet_converter_c::deliver_queued_packet() {
  if (!m_current_track->m_queued_packet)
    return;

  auto &packet = m_current_track->m_queued_packet;

  mxdebug_if(m_debug,
             fmt::format("  queue: delivering packet {0} duration {1} content {2}\n",
                         mtx::string::format_timestamp(packet->timestamp), mtx::string::format_timestamp(packet->duration), displayable_packet_content(*packet->data)));

  if (m_current_track->m_ptzr)
    m_current_track->m_ptzr->process(packet);
  packet.reset();
}

bool
teletext_to_srt_packet_converter_c::maybe_merge_queued_and_new_packet(packet_t const &new_packet) {
  if (!m_current_track->m_queued_packet)
    return false;

  auto const &old_content = *m_current_track->m_queued_packet->data;
  auto const &new_content = *new_packet.data;
  auto const same_content = old_content == new_content;

  auto prev_timestamp     = timestamp_c::ns(m_current_track->m_queued_packet->timestamp);
  auto prev_end           = prev_timestamp + timestamp_c::ns(m_current_track->m_queued_packet->duration);
  auto diff               = timestamp_c::ns(new_packet.timestamp) - prev_end;

  if (!same_content || (diff.abs() > s_merge_allowed_within)) {
    mxdebug_if(m_debug, fmt::format("  queue: not possible to merge; diff: {0} same content? {1}\n", diff, same_content));
    return false;
  }

  m_current_track->m_queued_packet->duration = (timestamp_c::ns(new_packet.timestamp + new_packet.duration) - prev_timestamp).abs().to_ns();
  mxdebug_if(m_debug,
             fmt::format("  queue: merging packet with previous, now {0} duration {1} content {2}\n",
                         mtx::string::format_timestamp(prev_timestamp), mtx::string::format_timestamp(m_current_track->m_queued_packet->duration), displayable_packet_content(new_content)));

  return true;
}

void
teletext_to_srt_packet_converter_c::queue_packet(packet_cptr const &new_packet) {
  if (maybe_merge_queued_and_new_packet(*new_packet))
    return;

  deliver_queued_packet();

  mxdebug_if(m_debug,
             fmt::format("  queue: queueing packet {0} duration {1} content {2}\n",
                         mtx::string::format_timestamp(new_packet->timestamp), mtx::string::format_timestamp(new_packet->duration), displayable_packet_content(*new_packet->data)));
  m_current_track->m_queued_packet = new_packet;
}

void
teletext_to_srt_packet_converter_c::flush() {
  for (auto const &pair : m_track_data) {
    auto &data = pair.second;
    if (!data->m_queued_packet)
      continue;

    mxdebug_if(m_debug,
               fmt::format("  queue: flushing packet {0} duration {1} content {2}\n",
                           mtx::string::format_timestamp(data->m_queued_packet->timestamp), mtx::string::format_timestamp(data->m_queued_packet->duration), displayable_packet_content(*data->m_queued_packet->data)));

    if (data->m_ptzr)
      data->m_ptzr->process(data->m_queued_packet);
    data->m_queued_packet.reset();
  }
}

void
teletext_to_srt_packet_converter_c::process_ttx_packet() {
  auto data_unit_id = m_buf[m_pos]; // 0x02 = teletext, 0x03 = subtitling, 0xff = stuffing
  auto start_byte   = m_buf[m_pos + 3];

  if (!mtx::included_in(data_unit_id, 0x02, 0x03) || (0xe4 != start_byte)) {
    if (0xff != data_unit_id)
      mxdebug_if(m_debug_packet, fmt::format(" m_pos {0} unsupported data_unit_id/start_byte 0x{1:02x} data_unit_id 0x{2:02x}\n", m_pos, static_cast<unsigned int>(start_byte), static_cast<unsigned int>(data_unit_id)));

    return;
  }

  bit_reverse(&m_buf[m_pos + 2], m_data_length);

  uint8_t ttx_header[2];
  unham(&m_buf[m_pos + 4], ttx_header, 2);

  auto ttx_header_magazine =  ttx_header[0] & 0x07;
  auto row_number          = (ttx_header[0] & 0xf8) >> 3;

  if (!ttx_header_magazine)
    ttx_header_magazine = 8;

  mxdebug_if(m_debug_packet, fmt::format(" m_pos {0} packet type (magazine+ID/row) {1}/{2}\n", m_pos, ttx_header_magazine, static_cast<unsigned int>(row_number)));

  if (row_number == 0) {
    decode_page_data(ttx_header_magazine);
    return;
  }

  if (   !m_current_track
      || (ttx_header_magazine != m_current_track->m_magazine)
      || (row_number          <  0)
      || (row_number          >= TTX_PAGE_TEXT_ROW_SIZE))
    return;

  process_single_row(row_number);

  if (!m_current_track->m_queued_timestamp.valid())
    m_current_track->m_queued_timestamp = m_current_track->m_page_timestamp;
}

std::string
teletext_to_srt_packet_converter_c::page_to_string()
  const {
  auto content = mtx::string::join(m_current_track->m_page_data.page_buffer, "\n");

  return to_utf8(Q(content).replace(m_page_re1, "\n").replace(m_page_re2, " ").replace(m_page_re3, {}));
}

bool
teletext_to_srt_packet_converter_c::convert(packet_cptr const &packet) {
  m_in_size                  = packet->data->get_size();
  m_buf                      = packet->data->get_buffer();
  m_pos                      = 1;                // skip sub ID
  m_current_packet_timestamp = timestamp_c::ns(packet->timestamp);

  mxdebug_if(m_debug_conversion, fmt::format("Starting conversion on packet with length {0} timestamp {1}\n", m_in_size, mtx::string::format_timestamp(packet->timestamp)));

  //
  // PES teletext payload (payload_index) packet length = 44 + 2 = 46
  //
  // byte 0    data unit id / (0x02 = teletext, 0x03 = subtitling, 0xff = stuffing)
  // byte 1    data length (0x2c = 44)
  // byte 2    original screen line number
  // byte 3    start byte = 0xe4
  // byte 4-5  hammed header byte 1
  // byte 6-45 text payload

  while ((m_pos + 6) < m_in_size) {
    m_data_length = m_buf[m_pos + 1];

    if ((m_pos + 2 + m_data_length) > m_in_size)
      break;

    if (m_data_length != 0x2c) {
      mxdebug_if(m_debug_conversion, fmt::format("pos {0} invalid data length {1} != {2}\n", m_pos, m_data_length, 0x2c));
      m_pos = m_pos + 2 + 0x2c;
      continue;
    }

    process_ttx_packet();

    m_pos += 2 + m_data_length;
  }

  return true;
}

void
teletext_to_srt_packet_converter_c::parse_for_probing() {
  m_parse_for_probing = true;
}

std::vector<int>
teletext_to_srt_packet_converter_c::get_probed_subtitle_page_numbers()
  const {
  std::vector<int> page_numbers;

  for (auto const &data : m_track_data)
    if (data.second->m_subtitle_page_found)
      page_numbers.push_back(data.first);

  return page_numbers;
}
