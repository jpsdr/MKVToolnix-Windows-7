/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   definitions and helper functions for AAC data

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef MTX_COMMON_AACCOMMON_H
#define MTX_COMMON_AACCOMMON_H

#include "common/common_pch.h"

#include "common/bit_cursor.h"
#include "common/byte_buffer.h"
#include "common/timecode.h"

#define AAC_ID_MPEG4 0
#define AAC_ID_MPEG2 1

#define AAC_PROFILE_MAIN 0
#define AAC_PROFILE_LC   1
#define AAC_PROFILE_SSR  2
#define AAC_PROFILE_LTP  3
#define AAC_PROFILE_SBR  4

#define AAC_SYNC_EXTENSION_TYPE 0x02b7

#define AAC_MAX_PRIVATE_DATA_SIZE 5

extern const int g_aac_sampling_freq[16];

class aac_header_c {
public:
  int object_type, extension_object_type, profile, sample_rate, output_sample_rate, bit_rate, channels, bytes;
  int id;                       // 0 = MPEG-4, 1 = MPEG-2
  int header_bit_size, header_byte_size, data_byte_size;

  bool is_sbr, is_valid;

protected:
  bit_reader_cptr m_bc;

public:
  aac_header_c();

  std::string to_string() const;

public:
  static aac_header_c from_audio_specific_config(const unsigned char *data, int size);

protected:
  int read_object_type();
  int read_sample_rate();
  void read_ga_specific_config();
  void read_program_config_element();
  void read_error_protection_specific_config();
  void parse(const unsigned char *data, int size);
};

bool operator ==(const aac_header_c &h1, const aac_header_c &h2);

namespace aac {

class frame_c {
public:
  unsigned int m_id, m_profile, m_sample_rate, m_bit_rate, m_channels, m_frame_size, m_header_bit_size, m_header_byte_size, m_data_byte_size;
  uint64_t m_stream_position;
  size_t m_garbage_size;
  timecode_c m_timecode;
  bool m_valid;
  memory_cptr m_data;

public:
  frame_c();
  void init();
  bool decode_adts_header(unsigned char const *buffer, size_t buffer_size);

  std::string to_string(bool verbose = false) const;

  int find_in(memory_cptr const &buffer);
  int find_in(unsigned char const *buffer, size_t buffer_size);
};

class parser_c {
public:
  enum multiplex_type_e {
      unknown_multiplex = 0
    , adts_multiplex
    , adif_multiplex
  };

protected:
  std::deque<frame_c> m_frames;
  std::deque<timecode_c> m_provided_timecodes;
  byte_buffer_c m_buffer;
  uint64_t m_parsed_stream_position, m_total_stream_position;
  size_t m_garbage_size;
  multiplex_type_e m_multiplex_type;

public:
  parser_c();
  void add_timecode(timecode_c const &timecode);
  void add_bytes(memory_cptr const &mem);
  void add_bytes(unsigned char *const buffer, size_t size);
  void flush();
  size_t frames_available() const;
  frame_c get_frame();
  uint64_t get_parsed_stream_position() const;
  uint64_t get_total_stream_position() const;

  int find_consecutive_frames(unsigned char const *buffer, size_t buffer_size, size_t num_required_headers);

protected:
  void parse();
  bool decode_header(unsigned char const *buffer, size_t buffer_size, frame_c &frame);
};
typedef std::shared_ptr<parser_c> parser_cptr;

}

bool parse_aac_adif_header(const unsigned char *buf, int size, aac_header_c *aac_header);
int find_aac_header(const unsigned char *buf, int size, aac_header_c *aac_header, bool emphasis_present);
int find_consecutive_aac_headers(const unsigned char *buf, int size, int num);

int get_aac_sampling_freq_idx(int sampling_freq);

bool parse_aac_data(const unsigned char *data, int size, int &profile, int &channels, int &sample_rate, int &output_sample_rate, bool &sbr);
int create_aac_data(unsigned char *data, int profile, int channels, int sample_rate, int output_sample_rate, bool sbr);
bool parse_aac_codec_id(const std::string &codec_id, int &id, int &profile);

#endif // MTX_COMMON_AACCOMMON_H
