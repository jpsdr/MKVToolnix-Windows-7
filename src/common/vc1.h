/** VC-1 video helper functions

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   \file

   \author Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include <deque>

#include "common/vc1_fwd.h"

namespace mtx::vc1 {

constexpr auto PROFILE_SIMPLE    = 0x00000000;
constexpr auto PROFILE_MAIN      = 0x00000001;
constexpr auto PROFILE_COMPLEX   = 0x00000002;
constexpr auto PROFILE_ADVANCED  = 0x00000003;

constexpr auto MARKER_ENDOFSEQ   = 0x0000010a;
constexpr auto MARKER_SLICE      = 0x0000010b;
constexpr auto MARKER_FIELD      = 0x0000010c;
constexpr auto MARKER_FRAME      = 0x0000010d;
constexpr auto MARKER_ENTRYPOINT = 0x0000010e;
constexpr auto MARKER_SEQHDR     = 0x0000010f;

enum frame_type_e {
  FRAME_TYPE_I,
  FRAME_TYPE_P,
  FRAME_TYPE_B,
  FRAME_TYPE_BI,
  FRAME_TYPE_P_SKIPPED,
};

enum frame_coding_mode_e {
  FCM_PROGRESSIVE = 0,
  FCM_ILACE_FRAME,
  FCM_ILACE_FIELD,
};

struct sequence_header_t {
  int  profile;
  int  level;
  int  chroma_format;
  int  frame_rtq_postproc;
  int  bit_rtq_postproc;
  bool postproc_flag;
  int  pixel_width;
  int  pixel_height;
  bool pulldown_flag;
  bool interlace_flag;
  bool tf_counter_flag;
  bool f_inter_p_flag;
  bool psf_mode_flag;
  bool display_info_flag;
  int  display_width;
  int  display_height;
  bool aspect_ratio_flag;
  int  aspect_ratio_width;
  int  aspect_ratio_height;
  bool framerate_flag;
  int  framerate_num;
  int  framerate_den;
  int  color_prim;
  int  transfer_char;
  int  matrix_coef;
  bool hrd_param_flag;
  int  hrd_num_leaky_buckets;

  sequence_header_t();
};

struct entrypoint_t {
  bool broken_link_flag;
  bool closed_entry_flag;
  bool pan_scan_flag;
  bool refdist_flag;
  bool loop_filter_flag;
  bool fast_uvmc_flag;
  bool extended_mv_flag;
  int  dquant;
  bool vs_transform_flag;
  bool overlap_flag;
  int  quantizer_mode;
  bool coded_dimensions_flag;
  int  coded_width;
  int  coded_height;
  bool extended_dmv_flag;
  bool luma_scaling_flag;
  int  luma_scaling;
  bool chroma_scaling_flag;
  int  chroma_scaling;

  entrypoint_t();
};

struct frame_header_t {
  int          fcm;
  frame_type_e frame_type;
  int          tf_counter;
  int          repeat_frame;
  bool         top_field_first_flag;
  bool         repeat_first_field_flag;

  frame_header_t();
  void init();
};

struct frame_t {
  frame_header_t header;
  memory_cptr    data;
  int64_t        timestamp;
  int64_t        duration;
  bool           contains_field, contains_entry_point;

  frame_t(frame_header_t const &p_header);
  void init();
  bool is_key() const;
};

inline bool is_marker(uint32_t value) {
  return (value & 0xffffff00) == 0x00000100;
}

inline bool is_fourcc(uint32_t value) {
  return mtx::calc_fourcc('W', 'V', 'C', '1') == value;
}

inline bool is_fourcc(const char *value) {
  return !strncasecmp(value, "WVC1", 4);
}

bool parse_sequence_header(const uint8_t *buf, int size, sequence_header_t &seqhdr);
bool parse_entrypoint(const uint8_t *buf, int size, entrypoint_t &entrypoint, sequence_header_t &seqhdr);
bool parse_frame_header(const uint8_t *buf, int size, frame_header_t &frame_header, sequence_header_t &seqhdr);

class es_parser_c {
protected:
  int64_t m_stream_pos;

  bool m_seqhdr_found;
  bool m_seqhdr_changed;
  sequence_header_t m_seqhdr;
  memory_cptr m_raw_seqhdr;
  memory_cptr m_raw_entrypoint;

  memory_cptr m_unparsed_buffer;

  std::deque<memory_cptr> m_pre_frame_extra_data;
  std::deque<memory_cptr> m_post_frame_extra_data;

  std::deque<frame_cptr> m_frames;
  frame_cptr m_current_frame;

  std::deque<memory_cptr> m_unparsed_packets;

  std::deque<int64_t> m_timestamps;
  std::deque<int64_t> m_timestamp_positions;
  int64_t m_previous_timestamp;
  int64_t m_num_timestamps;
  int64_t m_num_repeated_fields;

  bool m_default_duration_forced;
  int64_t m_default_duration;

public:
  es_parser_c();
  virtual ~es_parser_c() = default;

  virtual void add_bytes(uint8_t *buf, int size);
  virtual void add_bytes(memory_cptr const &buf) {
    add_bytes(buf->get_buffer(), buf->get_size());
  };

  virtual void flush();

  virtual bool is_sequence_header_available() const {
    return m_seqhdr_found;
  }

  virtual bool has_sequence_header_changed() const {
    return m_seqhdr_changed;
  }

  virtual bool are_headers_available() const {
    return m_seqhdr_found && m_raw_entrypoint;
  }

  virtual void get_sequence_header(sequence_header_t &seqhdr) const {
    if (m_seqhdr_found)
      memcpy(&seqhdr, &m_seqhdr, sizeof(sequence_header_t));
  }

  virtual memory_cptr get_raw_sequence_header() const {
    return m_seqhdr_found ? memory_cptr{m_raw_seqhdr->clone()} : memory_cptr{};
  }

  virtual memory_cptr get_raw_entrypoint() const {
    return m_raw_entrypoint ? m_raw_entrypoint->clone() : memory_cptr{};
  }

  virtual void handle_packet(memory_cptr const &packet);

  virtual bool is_frame_available() const {
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

  virtual void add_timestamp(int64_t timestamp, int64_t position);

  virtual void set_default_duration(int64_t default_duration) {
    m_default_duration        = default_duration;
    m_default_duration_forced = true;
  }

  virtual int64_t get_default_duration() const {
    return m_default_duration;
  }

protected:
  virtual void handle_end_of_sequence_packet(memory_cptr const &packet);
  virtual void handle_entrypoint_packet(memory_cptr const &packet);
  virtual void handle_field_packet(memory_cptr const &packet);
  virtual void handle_frame_packet(memory_cptr const &packet);
  virtual void handle_sequence_header_packet(memory_cptr const &packet);
  virtual void handle_slice_packet(memory_cptr const &packet);
  virtual void handle_unknown_packet(uint32_t marker, memory_cptr const &packet);

  virtual int64_t get_next_timestamp();
  virtual int64_t peek_next_calculated_timestamp() const;

  virtual void add_pre_frame_extra_data(memory_cptr const &packet);
  virtual void add_post_frame_extra_data(memory_cptr const &packet);
  virtual void combine_extra_data_with_packet();

  virtual bool postpone_processing(memory_cptr const &packet);
  virtual void process_unparsed_packets();

  virtual void flush_frame();

  virtual bool is_timestamp_available() const;

protected:
  static void add_extra_data_if_not_present(std::deque<memory_cptr> &extra_data, memory_cptr const &packet);
};

}
