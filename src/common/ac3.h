/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   definitions and helper functions for AC-3 data

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/byte_buffer.h"

class codec_c;

namespace mtx {

namespace bits {
class reader_c;
}

namespace ac3 {

constexpr auto SYNC_WORD              = 0x0b77;

constexpr auto FRAME_TYPE_INDEPENDENT = 0;
constexpr auto FRAME_TYPE_DEPENDENT   = 1;
constexpr auto FRAME_TYPE_AC3_CONVERT = 2;
constexpr auto FRAME_TYPE_RESERVED    = 3;

class frame_c {
public:
  unsigned int m_sample_rate{}, m_bit_rate{}, m_channels{}, m_channel_layout{}, m_flags{}, m_bytes{}, m_bs_id{}, m_samples{}, m_frame_type{}, m_sub_stream_id{};
  unsigned int m_dialog_normalization_gain{}, m_dialog_normalization_gain_bit_position{};
  std::optional<unsigned int> m_dialog_normalization_gain2, m_dialog_normalization_gain2_bit_position;
  uint64_t m_stream_position{}, m_garbage_size{};
  bool m_valid{}, m_lfeon{}, m_is_surround_ex{};
  memory_cptr m_data;
  std::vector<frame_c> m_dependent_frames;

public:
  void init();
  bool is_eac3() const;
  codec_c get_codec() const;
  void add_dependent_frame(frame_c const &frame, uint8_t const *buffer, std::size_t buffer_size);
  bool decode_header(uint8_t const *buffer, std::size_t buffer_size);
  bool decode_header_type_eac3(mtx::bits::reader_c &bc);
  bool decode_header_type_ac3(mtx::bits::reader_c &bc);

  uint64_t get_effective_channel_layout() const;
  int get_effective_number_of_channels() const;

  std::string to_string(bool verbose) const;

  int find_in(memory_cptr const &buffer);
  int find_in(uint8_t const *buffer, std::size_t buffer_size);
};

class parser_c {
protected:
  std::deque<frame_c> m_frames;
  mtx::bytes::buffer_c m_buffer;
  uint64_t m_parsed_stream_position, m_total_stream_position;
  frame_c m_current_frame;
  std::size_t m_garbage_size;

public:
  parser_c();
  void add_bytes(memory_cptr const &mem);
  void add_bytes(uint8_t *const buffer, std::size_t size);
  void flush();
  std::size_t frame_available() const;
  frame_c get_frame();
  uint64_t get_parsed_stream_position() const;
  uint64_t get_total_stream_position() const;

  int find_consecutive_frames(uint8_t const *buffer, std::size_t buffer_size, std::size_t num_required_headers);

  void parse(bool end_of_stream);
};

bool verify_checksums(uint8_t const *buf, std::size_t size, bool full_buffer = false);
void remove_dialog_normalization_gain(uint8_t *buf, std::size_t size);

}}                              // namespace mtx::ac3
