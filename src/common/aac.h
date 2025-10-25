/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   definitions and helper functions for AAC data

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <ostream>

#include "common/byte_buffer.h"
#include "common/timestamp.h"

namespace mtx::bits {
class reader_c;
class writer_c;
}

namespace mtx::aac {

constexpr auto ID_MPEG4              = 0;
constexpr auto ID_MPEG2              = 1;

constexpr auto PROFILE_MAIN          = 0;
constexpr auto PROFILE_LC            = 1;
constexpr auto PROFILE_SSR           = 2;
constexpr auto PROFILE_LTP           = 3;
constexpr auto PROFILE_SBR           = 4;

constexpr auto SYNC_EXTENSION_TYPE   = 0x02b7;

constexpr auto MAX_PRIVATE_DATA_SIZE = 5;

constexpr auto ADTS_SYNC_WORD        = 0xfff000;
constexpr auto ADTS_SYNC_WORD_MASK   = 0xfff000; // first 12 of 24 bits

constexpr auto LOAS_SYNC_WORD        = 0x56e000; // 0x2b7
constexpr auto LOAS_SYNC_WORD_MASK   = 0xffe000; // first 11 of 24 bits
constexpr auto LOAS_FRAME_SIZE_MASK  = 0x001fff; // last 13 of 24 bits

constexpr auto ID_PCE                = 0x05; // Table 4.71 "Syntactic elements"

struct audio_config_t {
  unsigned int profile{}, sample_rate{}, output_sample_rate{}, channels{}, samples_per_frame{1024};
  bool sbr{};
  memory_cptr ga_specific_config;
  unsigned int ga_specific_config_bit_size{};
  bool ga_specific_config_contains_program_config_element{};
};

unsigned int get_sampling_freq_idx(unsigned int sampling_freq);
bool parse_codec_id(const std::string &codec_id, int &id, int &profile);
std::optional<audio_config_t> parse_audio_specific_config(uint8_t const *data, std::size_t size);
void copy_program_config_element(mtx::bits::reader_c &r, mtx::bits::writer_c &w);
memory_cptr create_audio_specific_config(audio_config_t const &audio_config);

class header_c {
public:
  audio_config_t config{};

  unsigned int object_type{}, extension_object_type{}, bit_rate{}, bytes{};
  unsigned int id{};                       // 0 = MPEG-4, 1 = MPEG-2
  size_t header_bit_size{}, header_byte_size{}, data_byte_size{};

  bool is_valid{};

protected:
  mtx::bits::reader_c *m_bc{};

public:
  std::string to_string() const;

public:
  static header_c from_audio_specific_config(const uint8_t *data, size_t size);

  void parse_audio_specific_config(const uint8_t *data, size_t size, bool look_for_sync_extension = true);
  void parse_audio_specific_config(mtx::bits::reader_c &bc, bool look_for_sync_extension = true);
  void parse_program_config_element(mtx::bits::reader_c &bc);

protected:
  int read_object_type();
  int read_sample_rate();
  void read_eld_specific_config();
  void read_er_celp_specific_config();
  void read_ga_specific_config();
  void read_program_config_element();
  void read_error_protection_specific_config();
};

bool operator ==(const header_c &h1, const header_c &h2);

inline std::ostream &
operator <<(std::ostream &out,
            header_c const &header) {
  out << header.to_string();
  return out;
}

class latm_parser_c {
protected:
  int m_audio_mux_version, m_audio_mux_version_a;
  size_t m_fixed_frame_length, m_frame_length_type, m_frame_bit_offset, m_frame_length;
  header_c m_header;
  mtx::bits::reader_c *m_bc;
  bool m_config_parsed;
  memory_cptr m_audio_specific_config;
  debugging_option_c m_debug;

public:
  latm_parser_c();

  bool config_parsed() const;
  memory_cptr get_audio_specific_config() const;
  header_c const &get_header() const;
  size_t get_frame_bit_offset() const;
  size_t get_frame_length() const;

  void parse(mtx::bits::reader_c &bc);

protected:
  unsigned int get_value();
  void parse_audio_specific_config(size_t asc_length);
  void parse_stream_mux_config();
  void parse_payload_mux(size_t length);
  void parse_audio_mux_element();
  size_t parse_payload_length_info();
};

class frame_c {
public:
  header_c m_header;
  uint64_t m_stream_position;
  size_t m_garbage_size;
  timestamp_c m_timestamp;
  memory_cptr m_data;

public:
  frame_c();
  void init();

  std::string to_string(bool verbose = false) const;
};

class parser_c {
public:
  enum multiplex_type_e {
      unknown_multiplex = 0
    , adts_multiplex
    , adif_multiplex
    , loas_latm_multiplex
  };

protected:
  enum parse_result_e {
      failure
    , success
    , need_more_data
  };

protected:
  std::deque<frame_c> m_frames;
  std::deque<timestamp_c> m_provided_timestamps;
  mtx::bytes::buffer_c m_buffer, m_multiplex_type_detection_buffer;
  uint8_t const *m_fixed_buffer;
  size_t m_fixed_buffer_size;
  uint64_t m_parsed_stream_position, m_total_stream_position;
  size_t m_garbage_size, m_num_frames_found, m_abort_after_num_frames;
  bool m_require_frame_at_first_byte, m_copy_data;
  multiplex_type_e m_multiplex_type;
  header_c m_header;
  latm_parser_c m_latm_parser;
  debugging_option_c m_debug;

public:
  parser_c();
  void add_timestamp(timestamp_c const &timestamp);

  void add_bytes(memory_cptr const &mem);
  void add_bytes(uint8_t const *buffer, size_t size);

  void parse_fixed_buffer(uint8_t const *fixed_buffer, size_t fixed_buffer_size);
  void parse_fixed_buffer(memory_cptr const &fixed_buffer);

  void flush();

  multiplex_type_e get_multiplex_type() const;
  void set_multiplex_type(multiplex_type_e multiplex_type);

  size_t frames_available() const;
  bool headers_parsed() const;

  frame_c get_frame();
  uint64_t get_parsed_stream_position() const;
  uint64_t get_total_stream_position() const;
  memory_cptr get_audio_specific_config() const;

  void abort_after_num_frames(size_t num_frames);
  void require_frame_at_first_byte(bool require);
  void copy_data(bool copy);

public:                         // static functions
  static int find_consecutive_frames(uint8_t const *buffer, size_t buffer_size, size_t num_required_frames);
  static std::string get_multiplex_type_name(multiplex_type_e multiplex_type);

protected:
  void parse();
  std::pair<parse_result_e, size_t> decode_header(uint8_t const *buffer, size_t buffer_size);
  std::pair<parse_result_e, size_t> decode_adts_header(uint8_t const *buffer, size_t buffer_size);
  std::pair<parse_result_e, size_t> decode_loas_latm_header(uint8_t const *buffer, size_t buffer_size);
  bool determine_multiplex_type(uint8_t const *buffer, std::size_t buffer_size);
  void push_frame(frame_c &frame);
};
using parser_cptr = std::shared_ptr<parser_c>;

} // namespace mtx::aac

#if FMT_VERSION >= 90000
template <> struct fmt::formatter<mtx::aac::header_c> : ostream_formatter {};
#endif  // FMT_VERSION >= 90000
