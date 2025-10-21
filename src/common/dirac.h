/** MPEG video helper functions (MPEG 1, 2 and 4)

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   \file

   \author Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx::dirac {

constexpr auto SYNC_WORD            = 0x42424344; // 'BBCD'
constexpr auto UNIT_SEQUENCE_HEADER = 0x00;
constexpr auto UNIT_END_OF_SEQUENCE = 0x10;
constexpr auto UNIT_AUXILIARY_DATA  = 0x20;
constexpr auto UNIT_PADDING         = 0x30;

inline bool is_auxiliary_data(int type) {
  return (type & 0xf8) == 0x20;
}

inline bool is_picture(int type) {
  return (type & 0x08) == 0x08;
}

struct sequence_header_t {
  unsigned int major_version;
  unsigned int minor_version;
  unsigned int profile;
  unsigned int level;
  unsigned int base_video_format;
  unsigned int pixel_width;
  unsigned int pixel_height;
  unsigned int chroma_format;
  bool         interlaced;
  bool         top_field_first;
  unsigned int frame_rate_numerator;
  unsigned int frame_rate_denominator;
  unsigned int aspect_ratio_numerator;
  unsigned int aspect_ratio_denominator;
  unsigned int clean_width;
  unsigned int clean_height;
  unsigned int left_offset;
  unsigned int top_offset;

  sequence_header_t();
};

struct frame_t {
  memory_cptr data;
  int64_t     timestamp;
  int64_t     duration;
  bool        contains_sequence_header;

  frame_t();
  void init();
};
using frame_cptr = std::shared_ptr<frame_t>;

bool parse_sequence_header(const uint8_t *buf, int size, sequence_header_t &seqhdr);

class es_parser_c {
protected:
  int64_t m_stream_pos;

  bool m_seqhdr_found;
  bool m_seqhdr_changed;
  sequence_header_t m_seqhdr;
  memory_cptr m_raw_seqhdr;

  memory_cptr m_unparsed_buffer;

  std::deque<memory_cptr> m_pre_frame_extra_data;
  std::deque<memory_cptr> m_post_frame_extra_data;

  std::deque<frame_cptr> m_frames;
  frame_cptr m_current_frame;

  std::deque<int64_t> m_timestamps;
  int64_t m_previous_timestamp;
  int64_t m_num_timestamps;
  int64_t m_num_repeated_fields;

  bool m_default_duration_forced;
  int64_t m_default_duration;

public:
  es_parser_c();
  virtual ~es_parser_c() = default;

  virtual void add_bytes(uint8_t *buf, size_t size);
  virtual void add_bytes(memory_cptr &buf) {
    add_bytes(buf->get_buffer(), buf->get_size());
  };

  virtual void flush();

  virtual bool is_sequence_header_available() {
    return m_seqhdr_found;
  }

  virtual bool has_sequence_header_changed() {
    return m_seqhdr_changed;
  }

  virtual bool are_headers_available() {
    return m_seqhdr_found;
  }

  virtual void get_sequence_header(sequence_header_t &seqhdr) {
    if (m_seqhdr_found)
      memcpy(&seqhdr, &m_seqhdr, sizeof(sequence_header_t));
  }

  virtual memory_cptr get_raw_sequence_header() {
    return m_seqhdr_found ? memory_cptr{m_raw_seqhdr->clone()} : memory_cptr{};
  }

  virtual void handle_unit(memory_cptr const &packet);

  virtual bool is_frame_available() {
    return !m_frames.empty();
  }

  virtual frame_cptr get_frame() {
    frame_cptr frame;

    if (!m_frames.empty()) {
      frame = m_frames.front();
      m_frames.pop_front();
    }

    return frame;
  }

  virtual void add_timestamp(int64_t timestamp) {
    m_timestamps.push_back(timestamp);
  }

  virtual void set_default_duration(int64_t default_duration) {
    m_default_duration        = default_duration;
    m_default_duration_forced = true;
  }

  virtual int64_t get_default_duration() {
    return m_default_duration;
  }

protected:
  virtual void handle_auxiliary_data_unit(memory_c const &packet);
  virtual void handle_end_of_sequence_unit(memory_c const &packet);
  virtual void handle_padding_unit(memory_c const &packet);
  virtual void handle_picture_unit(memory_cptr const &packet);
  virtual void handle_sequence_header_unit(memory_c const &packet);
  virtual void handle_unknown_unit(memory_c const &packet);

  virtual int64_t get_next_timestamp();
  virtual int64_t peek_next_calculated_timestamp();

  virtual void add_pre_frame_extra_data(memory_c const &packet);
  virtual void add_post_frame_extra_data(memory_c const &packet);
  virtual void combine_extra_data_with_packet();

  virtual void flush_frame();
};

} // namespace mtx::dirac
