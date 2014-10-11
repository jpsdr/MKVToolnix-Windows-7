/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Conversion from teletext to SRT subtitles

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/strings/formatting.h"
#include "merge/pr_generic.h"
#include "input/teletext_to_srt_packet_converter.h"

std::vector<teletext_to_srt_packet_converter_c::char_map_t> teletext_to_srt_packet_converter_c::ms_char_maps;

static unsigned char invtab[256] = {
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

static unsigned char unhamtab[256] = {
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

void
teletext_to_srt_packet_converter_c::ttx_page_data_t::reset() {
  page         = -1;
  subpage      = -1;
  flags        =  0;
  national_set =  0;
  erase_flag   = false;

  page_buffer.clear();
  page_buffer.reserve(TTX_PAGE_ROW_SIZE);
  for (auto idx = 0; idx < TTX_PAGE_ROW_SIZE; ++idx)
    page_buffer.emplace_back();
}

teletext_to_srt_packet_converter_c::teletext_to_srt_packet_converter_c(generic_packetizer_c *ptzr)
  : packet_converter_c{ptzr}
  , m_in_size{}
  , m_pos{}
  , m_data_length{}
  , m_buf{}
  , m_previous_timecode{-1}
  , m_page_re1{" *\\n[ \\n]+",      boost::regex::perl}
  , m_page_re2{" +",                boost::regex::perl}
  , m_page_re3{"^[ \\n]+|[ \\n]+$", boost::regex::perl}
  , m_debug{"teletext_to_srt|teletext_to_srt_packet_converter"}
{
  m_ttx_page_data.reset();
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
teletext_to_srt_packet_converter_c::bit_reverse(unsigned char *buffer,
                                                size_t length) {
  for (size_t idx = 0; idx < length; ++idx)
    buffer[idx] = invtab[buffer[idx]];
}

void
teletext_to_srt_packet_converter_c::unham(unsigned char const *in,
                                          unsigned char *out,
                                          size_t hambytes) {
  for (size_t idx = 0; idx < (hambytes / 2); ++idx, ++out, in += 2)
    *out = (unhamtab[*in] & 0x0f) | ((unhamtab[*(in + 1)] & 0x0f) << 4);
}


int
teletext_to_srt_packet_converter_c::ttx_to_page(int ttx) {
  int retval = 0;

  for (size_t idx = 0; idx < 4; ++idx) {
    retval *= 10;
    retval += (ttx & 0xf000) >> 12;
    ttx   <<= 4;
  }

  return retval;
}

void
teletext_to_srt_packet_converter_c::remove_parity(unsigned char *buffer,
                                                  size_t length) {
  for (size_t idx = 0; idx < length; ++idx)
    buffer[idx] &= 0x7f;
}

void
teletext_to_srt_packet_converter_c::decode_line(unsigned char const *buffer,
                                                unsigned int row) {
  setup_character_maps();

  auto &recoded = m_ttx_page_data.page_buffer[row];

  if (m_ttx_page_data.national_set >= ms_char_maps.size()) {
    recoded = std::string{reinterpret_cast<char const *>(buffer), TTX_PAGE_COL_SIZE};
    return;
  }

  auto &char_map = ms_char_maps[m_ttx_page_data.national_set];

  recoded.clear();

  for (auto idx = 0u; idx < TTX_PAGE_COL_SIZE; ++idx) {
    auto c      = buffer[idx];
    auto mapped = char_map[static_cast<int>(c)];

    recoded    += mapped  ? std::string{mapped}
                : c < ' ' ? std::string{' '}
                :           std::string{static_cast<char>(c)};
  }
}

void
teletext_to_srt_packet_converter_c::process_ttx_packet(packet_cptr const &packet) {
  auto data_unit_id = m_buf[m_pos]; // 0x02 = teletext, 0x03 = subtitling, 0xff = stuffing
  auto start_byte   = m_buf[m_pos + 3];

  if ((0x03 != data_unit_id) || (0xe4 != start_byte)) {
    if ((0xff != data_unit_id) && (0x02 != data_unit_id))
      mxdebug_if(m_debug, boost::format("m_pos %1% data_unit_id 0x%|2$02x| start_byte 0x%|3$02x|\n") % m_pos % static_cast<unsigned int>(data_unit_id) % static_cast<unsigned int>(start_byte));

    return;
  }

  bit_reverse(&m_buf[m_pos + 2], m_data_length);

  unsigned char ttx_header[2];
  unham(&m_buf[m_pos + 4], ttx_header, 2);

  auto ttx_header_magazine =  ttx_header[0] & 0x07;
  auto row_number          = (ttx_header[0] & 0xf8) >> 3;

  if (!ttx_header_magazine)
    ttx_header_magazine = 8;

  mxdebug_if(m_debug, boost::format("m_pos %1% packet_id %2%\n") % m_pos % static_cast<unsigned int>(row_number));

  if (!row_number) {
    unsigned char ttx_packet_0_header[4];
    unham(&m_buf[m_pos + 6], ttx_packet_0_header, 8);

    if (ttx_packet_0_header[0] != 0xff) {
      m_ttx_page_data.page  = ttx_to_page(ttx_packet_0_header[0]);
      m_ttx_page_data.page += 100 * ttx_header_magazine;
    }

    m_ttx_page_data.subpage      = ((ttx_packet_0_header[2] << 8) | (ttx_packet_0_header[1])) & 0x3f7f;
    m_ttx_page_data.subpage      = ttx_to_page(m_ttx_page_data.subpage);

    m_ttx_page_data.flags        =  (ttx_packet_0_header[1]       & 0x80)
                                 | ((ttx_packet_0_header[3] << 4) & 0x10)
                                 | ((ttx_packet_0_header[3] << 2) & 0x08)
                                 | ((ttx_packet_0_header[3] >> 0) & 0x04)
                                 | ((ttx_packet_0_header[3] >> 1) & 0x02)
                                 | ((ttx_packet_0_header[3] >> 4) & 0x01);

    m_ttx_page_data.erase_flag   = !!(m_ttx_page_data.flags & 0x80);
    m_ttx_page_data.national_set = (m_ttx_page_data.flags >> 21) & 0x07;

    mxdebug_if(m_debug, boost::format("  ttx page %1% subpage %2% erase? %3% national set %4%\n") % m_ttx_page_data.page % m_ttx_page_data.subpage % m_ttx_page_data.erase_flag % m_ttx_page_data.national_set);

    auto current_content = page_to_string();
    mxdebug_if(m_debug,
               boost::format("  case !packet_id. page content before clearing it at timecode %2% (prev %3%): %1% PREV content: %4%\n")
               % current_content % format_timecode(packet->timecode) % format_timecode(m_previous_timecode) % m_previous_content);

    if (!m_previous_content.empty()) {
      m_previous_timecode = std::max<int64_t>(m_previous_timecode, 0);
      auto new_packet     = std::make_shared<packet_t>(memory_c::clone(m_previous_content), m_previous_timecode, std::abs(packet->timecode - m_previous_timecode));

      mxdebug_if(m_debug, boost::format("  WILL DELIVER at %1% duration %2% content %3%\n") % format_timecode(m_previous_timecode) % format_timecode(new_packet->duration) % m_previous_content);

      m_ptzr->process(new_packet);

      m_previous_timecode = -1;
      // packet->timecode    = -1;
    }

    if (!current_content.empty() && (m_previous_timecode == -1))
      m_previous_timecode = packet->timecode;

    m_ttx_page_data.reset();
    m_previous_content = current_content;

  } else if ((row_number > 0) && (row_number <= TTX_PAGE_ROW_SIZE)) {
    remove_parity(&m_buf[m_pos + 6], m_data_length + 2 - 6);
    decode_line(&m_buf[m_pos + 6], row_number);
  }
}

std::string
teletext_to_srt_packet_converter_c::page_to_string()
  const {
  auto content = join("\n", m_ttx_page_data.page_buffer);
  for (auto &c : content)
    if ((c < ' ') && (c != '\n'))
      c = ' ';

  return boost::regex_replace(boost::regex_replace(boost::regex_replace(content, m_page_re1, "\\n"), m_page_re2, " "), m_page_re3, "", boost::match_single_line);
}

bool
teletext_to_srt_packet_converter_c::convert(packet_cptr const &packet) {
  m_in_size  = packet->data->get_size();
  m_buf      = packet->data->get_buffer();
  m_pos      = 1;                // skip sub ID

  // m_ttx_page_data.reset();

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

    if ((m_pos + 2 + m_data_length) >= m_in_size)
      break;

    if (m_data_length != 0x2c) {
      mxdebug_if(m_debug, boost::format("pos %1% invalid data length %2% != %3%\n") % m_data_length % 0x2c);
      m_pos = m_pos + 2 + 0x2c;
      continue;
    }

    process_ttx_packet(packet);

    m_pos += 2 + m_data_length;
  }

  if ((-1 == m_previous_timecode) && (-1 != packet->timecode) && !page_to_string().empty())
    m_previous_timecode = packet->timecode;

  // mxdebug_if(m_debug, boost::format("At end of PES packet. Page content at timecode %2% (prev %3%): %1%\n") % page_to_string() % format_timecode(packet->timecode) % format_timecode(m_timecode));

  return true;
}
