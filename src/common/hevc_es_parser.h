/** HEVC video helper functions

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   \file

*/

#pragma once

#include "common/common_pch.h"

#include "common/hevc_types.h"
#include "common/math_fwd.h"
#include "common/dovi_meta.h"

namespace mtx::hevc {

struct slice_info_t {
  unsigned char nalu_type{};
  unsigned char slice_type{};
  unsigned char pps_id{};
  bool first_slice_segment_in_pic_flag{};
  unsigned int pic_order_cnt_lsb{};

  unsigned int sps{};
  unsigned int pps{};
  int temporal_id{};

  void dump() const;
  void clear();
};

struct frame_t {
  memory_cptr m_data{};
  int64_t m_start{}, m_end{}, m_ref1{}, m_ref2{};
  uint64_t m_position{};
  bool m_keyframe{}, m_has_provided_timestamp{};
  slice_info_t m_si{};
  int m_presentation_order{}, m_decode_order{};

  void clear();
};

class es_parser_c {
protected:
  enum class extra_data_position_e {
    pre,
    initial,
    dont_store,
  };

  int m_nalu_size_length{4};

  bool m_keep_ar_info{true};
  bool m_hevcc_ready{}, m_hevcc_changed{};

  int64_t m_stream_default_duration{-1}, m_forced_default_duration{-1}, m_container_default_duration{-1};
  int m_frame_number{}, m_num_skipped_frames{};
  bool m_first_keyframe_found{}, m_recovery_point_valid{}, m_b_frames_since_keyframe{}, m_first_cleanup{true};
  int m_prev_pic_order_cnt_lsb{}, m_prev_pic_order_cnt_msb{};

  bool m_par_found{};
  mtx_mp_rational_t m_par{};

  std::deque<frame_t> m_frames, m_frames_out;
  std::deque<std::pair<int64_t, uint64_t>> m_provided_timestamps;
  int64_t m_max_timestamp{};
  std::map<int64_t, int64_t> m_duration_frequency;

  std::vector<memory_cptr> m_vps_list, m_sps_list, m_pps_list, m_extra_data_pre, m_extra_data_initial, m_pending_frame_data;
  std::vector<vps_info_t> m_vps_info_list;
  std::vector<sps_info_t> m_sps_info_list;
  std::vector<pps_info_t> m_pps_info_list;
  user_data_t m_user_data;
  codec_private_t m_codec_private;

  mtx::dovi::dovi_rpu_data_header_t m_dovi_rpu_data_header;

  memory_cptr m_unparsed_buffer;
  uint64_t m_stream_position{}, m_parsed_position{};

  frame_t m_incomplete_frame;
  std::deque<std::pair<memory_cptr, uint64_t>> m_unhandled_nalus;

  bool m_simple_picture_order{}, m_discard_actual_frames{}, m_normalize_parameter_sets{};

  debugging_option_c m_debug_keyframe_detection{"hevc_parser|hevc_keyframe_detection"}, m_debug_nalu_types{"hevc_parser|hevc_nalu_types"}, m_debug_timestamps{"hevc_parser|hevc_timestamps"}, m_debug_sps_info{"hevc_parser|hevc_sps|hevc_sps_info"}, m_debug_parameter_sets{"hevc_parser|hevc_parameter_sets"},
    m_debug_frame_order{"hevc_parser|hevc_frame_order"};
  static std::unordered_map<int, std::string> ms_nalu_names_by_type;

  struct stats_t {
    std::vector<int> num_slices_by_type, num_nalus_by_type;
    size_t num_frames_out{}, num_frames_discarded{}, num_timestamps_in{}, num_timestamps_generated{}, num_timestamps_discarded{}, num_field_slices{}, num_frame_slices{};

    stats_t()
      : num_slices_by_type(3, 0)
      , num_nalus_by_type(64, 0)
    {
    }
  } m_stats;

public:
  es_parser_c();
  ~es_parser_c();

  void force_default_duration(int64_t default_duration) {
    m_forced_default_duration = default_duration;
  }

  void set_container_default_duration(int64_t default_duration) {
    m_container_default_duration = default_duration;
  }

  void set_keep_ar_info(bool keep) {
    m_keep_ar_info = keep;
  }

  void set_next_i_slice_is_key_frame() {
    m_recovery_point_valid = true;
  }

  void add_bytes(unsigned char *buf, size_t size);
  void add_bytes(memory_cptr &buf) {
    add_bytes(buf->get_buffer(), buf->get_size());
  }

  void add_bytes_framed(unsigned char *buf, size_t buffer_size, size_t nalu_size_length);
  void add_bytes_framed(memory_cptr &buf, size_t nalu_size_length) {
    add_bytes_framed(buf->get_buffer(), buf->get_size(), nalu_size_length);
  }

  void flush();
  void clear();

  bool frame_available() {
    return !m_frames_out.empty();
  }

  frame_t get_frame() {
    assert(!m_frames_out.empty());

    frame_t frame(*m_frames_out.begin());
    m_frames_out.erase(m_frames_out.begin(), m_frames_out.begin() + 1);

    return frame;
  }

  void set_hevcc(memory_cptr const &hevcc_bytes);
  memory_cptr get_hevcc() const;

  bool hevcc_changed() const {
    return m_hevcc_changed;
  }

  int get_width() const {
    assert(!m_sps_info_list.empty());
    return m_sps_info_list.front().get_width();
  }

  int get_height() const {
    assert(!m_sps_info_list.empty());
    return m_sps_info_list.front().get_height();
  }

  void handle_nalu(memory_cptr const &nalu, uint64_t nalu_pos);

  void add_timestamp(int64_t timestamp);

  bool headers_parsed() const;

  void set_nalu_size_length(int nalu_size_length) {
    m_nalu_size_length = nalu_size_length;
  }

  int get_nalu_size_length() const {
    return m_nalu_size_length;
  }

  void discard_actual_frames(bool discard = true);
  void normalize_parameter_sets(bool normalize = true);

  int get_num_skipped_frames() const {
    return m_num_skipped_frames;
  }

  void dump_info() const;

  bool has_stream_default_duration() const {
    return -1 != m_stream_default_duration;
  }
  int64_t get_stream_default_duration() const {
    assert(-1 != m_stream_default_duration);
    return m_stream_default_duration;
  }

  int64_t duration_for(slice_info_t const &si) const;
  int64_t get_most_often_used_duration() const;

  size_t get_num_field_slices() const;
  size_t get_num_frame_slices() const;

  bool has_par_been_found() const;
  mtx_mp_rational_t const &get_par() const;
  std::pair<int64_t, int64_t> const get_display_dimensions(int width = -1, int height = -1) const;

  static std::string get_nalu_type_name(int type);

  bool has_dovi_rpu_header() const {
    return m_dovi_rpu_data_header.rpu_nal_prefix == 25;
  }

  mtx::dovi::dovi_rpu_data_header_t get_dovi_rpu_header() const {
    return m_dovi_rpu_data_header;
  }

  vui_info_t get_vui_info() const {
    assert(!m_sps_info_list.empty());
    return m_sps_info_list.front().vui;
  }

protected:
  bool parse_slice(memory_cptr const &nalu, slice_info_t &si);
  void handle_nalu_internal(memory_cptr const &nalu, uint64_t nalu_pos);
  void handle_vps_nalu(memory_cptr const &nalu, extra_data_position_e extra_data_position = extra_data_position_e::pre);
  void handle_sps_nalu(memory_cptr const &nalu, extra_data_position_e extra_data_position = extra_data_position_e::pre);
  void handle_pps_nalu(memory_cptr const &nalu, extra_data_position_e extra_data_position = extra_data_position_e::pre);
  void handle_sei_nalu(memory_cptr const &nalu, extra_data_position_e extra_data_position = extra_data_position_e::pre);
  void handle_unspec62_nalu(memory_cptr const &nalu);
  void handle_slice_nalu(memory_cptr const &nalu, uint64_t nalu_pos);
  void cleanup();
  void flush_incomplete_frame();
  void flush_unhandled_nalus();
  std::vector<int64_t> calculate_provided_timestamps_to_use();
  void calculate_frame_order();
  void calculate_frame_timestamps();
  void calculate_frame_references_and_update_stats();
  void add_parameter_sets_to_extra_data();
  void add_nalu_to_extra_data(memory_cptr const &nalu, extra_data_position_e position = extra_data_position_e::pre);
  void add_nalu_to_pending_frame_data(memory_cptr const &nalu);
  void build_frame_data();

  void maybe_dump_raw_data(unsigned char const *buffer, std::size_t size);

  static void init_nalu_names();
};

}                              // namespace mtx::hevc
