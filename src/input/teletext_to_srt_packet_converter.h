/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definitions for the converter from teletext to SRT subtitles

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef MTX_INPUT_TELETEXT_TO_SRT_PACKET_CONVERTER_H
#define MTX_INPUT_TELETEXT_TO_SRT_PACKET_CONVERTER_H

#include "common/common_pch.h"

#include "common/debugging.h"
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

  typedef std::unordered_map<int, char const *> char_map_t;
  static std::vector<char_map_t> ms_char_maps;

protected:
  size_t m_in_size, m_pos, m_data_length;
  unsigned char *m_buf;
  int64_t m_previous_timecode;
  std::string m_previous_content;

  ttx_page_data_t m_ttx_page_data;

  boost::regex m_page_re1, m_page_re2, m_page_re3;

  debugging_option_c m_debug;

public:
  teletext_to_srt_packet_converter_c(generic_packetizer_c *ptzr);
  virtual ~teletext_to_srt_packet_converter_c() {};

  virtual bool convert(packet_cptr const &packet);

protected:
  void process_ttx_packet(packet_cptr const &packet);
  std::string page_to_string() const;
  void decode_line(unsigned char const *buffer, unsigned int row);

protected:
  static int ttx_to_page(int ttx);
  static void bit_reverse(unsigned char *buffer, size_t length);
  static void unham(unsigned char const *in, unsigned char *out, size_t hambytes);
  static void remove_parity(unsigned char *buffer, size_t length);
  static void setup_character_maps();
};

#endif  // MTX_INPUT_TELETEXT_TO_SRT_PACKET_CONVERTER_H
