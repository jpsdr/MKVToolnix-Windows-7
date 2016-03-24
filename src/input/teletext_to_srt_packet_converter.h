/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definitions for the converter from teletext to SRT subtitles

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef MTX_INPUT_TELETEXT_TO_SRT_PACKET_CONVERTER_H
#define MTX_INPUT_TELETEXT_TO_SRT_PACKET_CONVERTER_H

#include "common/common_pch.h"

#include "common/debugging.h"
#include "common/timestamp.h"
#include "input/packet_converter.h"

#define TTX_PAGE_ROW_SIZE 24
#define TTX_PAGE_COL_SIZE 40

class teletext_to_srt_packet_converter_c: public packet_converter_c {
public:
  struct ttx_page_data_t {
    int page, subpage;
    unsigned int flags, national_set;
    bool erase_flag;
    std::vector<std::string> page_buffer;

    void reset();
  };

  using char_map_t = std::unordered_map<int, char const *>;
  static std::vector<char_map_t> ms_char_maps;

protected:
  size_t m_in_size, m_pos, m_data_length;
  unsigned char *m_buf;
  timestamp_c m_queued_timestamp, m_current_page_timestamp, m_current_packet_timestamp;
  packet_cptr m_queued_packet;
  boost::optional<int> m_wanted_page;
  bool m_page_changed{};

  ttx_page_data_t m_ttx_page_data;

  boost::regex m_page_re1, m_page_re2, m_page_re3;

  debugging_option_c m_debug;

public:
  teletext_to_srt_packet_converter_c(generic_packetizer_c *ptzr);
  virtual ~teletext_to_srt_packet_converter_c() {};

  virtual bool convert(packet_cptr const &packet) override;
  virtual void flush() override;

protected:
  void process_ttx_packet();
  std::string page_to_string() const;
  bool decode_line(unsigned char const *buffer, unsigned int row_number);
  void process_single_row(unsigned int row_number);
  void decode_page_data(unsigned char ttx_header_magazine);
  void deliver_queued_content();
  void queue_packet(packet_cptr const &new_packet);

protected:
  static int ttx_to_page(int ttx);
  static void bit_reverse(unsigned char *buffer, size_t length);
  static void unham(unsigned char const *in, unsigned char *out, size_t hambytes);
  static void remove_parity(unsigned char *buffer, size_t length);
  static void setup_character_maps();
};

#endif  // MTX_INPUT_TELETEXT_TO_SRT_PACKET_CONVERTER_H
