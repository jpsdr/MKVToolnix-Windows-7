/** MPEG video helper functions (MPEG 1, 2 and 4)

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   \file

   \author Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/avc.h"

namespace mtx { namespace avc {

struct frame_t {
  memory_cptr m_data;
  int64_t m_start, m_end, m_ref1, m_ref2;
  uint64_t m_position;
  bool m_keyframe, m_has_provided_timestamp;
  slice_info_t m_si;
  int m_presentation_order, m_decode_order;
  char m_type;
  bool m_order_calculated;

  frame_t() {
    clear();
  }

  frame_t(const frame_t &f) {
    *this = f;
  }

  void clear() {
    m_start                  = 0;
    m_end                    = 0;
    m_ref1                   = 0;
    m_ref2                   = 0;
    m_position               = 0;
    m_keyframe               = false;
    m_has_provided_timestamp = false;
    m_presentation_order     = 0;
    m_decode_order           = 0;
    m_type                   = '?';
    m_order_calculated       = false;
    m_data.reset();

    m_si.clear();
  }

  bool is_i_frame() const {
    return 'I' == m_type;
  }

  bool is_p_frame() const {
    return 'P' == m_type;
  }

  bool is_b_frame() const {
    return 'B' == m_type;
  }

  bool is_discardable() const {
    return m_si.nal_ref_idc == 0;
  }
};

class es_parser_c {
protected:
  int m_nalu_size_length;

  bool m_keep_ar_info, m_fix_bitstream_frame_rate;
  bool m_avcc_ready, m_avcc_changed, m_sps_or_sps_overwritten{};

  int64_t m_stream_default_duration, m_forced_default_duration, m_container_default_duration;
  int m_frame_number, m_num_skipped_frames;
  bool m_first_keyframe_found, m_recovery_point_valid, m_b_frames_since_keyframe;
  boost::optional<bool> m_current_key_frame_bottom_field;

  bool m_par_found;
  int64_rational_c m_par;

  std::deque<frame_t> m_frames, m_frames_out;
  std::deque<std::pair<int64_t, uint64_t>> m_provided_timestamps;
  int64_t m_max_timestamp, m_previous_i_p_start;
  std::map<int64_t, int64_t> m_duration_frequency;

  std::vector<memory_cptr> m_sps_list, m_pps_list, m_extra_data;
  std::vector<sps_info_t> m_sps_info_list;
  std::vector<pps_info_t> m_pps_info_list;

  memory_cptr m_unparsed_buffer;
  uint64_t m_stream_position, m_parsed_position;

  frame_t m_incomplete_frame;
  bool m_have_incomplete_frame;
  std::deque<std::pair<memory_cptr, uint64_t>> m_unhandled_nalus;

  bool m_ignore_nalu_size_length_errors, m_discard_actual_frames, m_simple_picture_order, m_first_cleanup, m_all_i_slices_are_key_frames;

  debugging_option_c m_debug_keyframe_detection, m_debug_nalu_types, m_debug_timestamps, m_debug_sps_info, m_debug_sps_pps_changes;
  static std::map<int, std::string> ms_nalu_names_by_type;

  struct stats_t {
    std::vector<int> num_slices_by_type, num_nalus_by_type;
    size_t num_frames_out{}, num_frames_discarded{}, num_timestamps_in{}, num_timestamps_generated{}, num_timestamps_discarded{}, num_field_slices{}, num_frame_slices{}, num_sei_nalus{}, num_idr_slices{};

    stats_t()
      : num_slices_by_type(11, 0)
      , num_nalus_by_type(13, 0)
    {
    }
  } m_stats;

public:
  es_parser_c();
  ~es_parser_c();

  void force_default_duration(int64_t default_duration) {
    m_forced_default_duration = default_duration;
  }
  bool is_default_duration_forced() const {
    return -1 != m_forced_default_duration;
  }

  void set_container_default_duration(int64_t default_duration) {
    m_container_default_duration = default_duration;
  }

  bool has_timing_info() const {
    return !m_sps_info_list.empty() && m_sps_info_list[0].timing_info_valid();
  }

  timing_info_t get_timing_info() const {
    if (!has_timing_info())
      throw std::out_of_range{"no timing information present"};
    return m_sps_info_list[0].timing_info;
  }

  void set_keep_ar_info(bool keep) {
    m_keep_ar_info = keep;
  }

  void set_fix_bitstream_frame_rate(bool fix) {
    m_fix_bitstream_frame_rate = fix;
  }

  void add_bytes(unsigned char *buf, size_t size);
  void add_bytes(memory_cptr &buf) {
    add_bytes(buf->get_buffer(), buf->get_size());
  }

  void flush();

  bool frame_available() {
    return !m_frames_out.empty();
  }

  frame_t get_frame() {
    assert(!m_frames_out.empty());

    frame_t frame(*m_frames_out.begin());
    m_frames_out.erase(m_frames_out.begin(), m_frames_out.begin() + 1);

    return frame;
  }

  memory_cptr get_avcc() const;

  bool avcc_changed() const {
    return m_avcc_changed;
  }

  int get_width() const {
    assert(!m_sps_info_list.empty());
    return m_sps_info_list.begin()->width;
  }

  int get_height() const {
    assert(!m_sps_info_list.empty());
    return m_sps_info_list.begin()->height;
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

  void ignore_nalu_size_length_errors() {
    m_ignore_nalu_size_length_errors = true;
  }

  void discard_actual_frames(bool discard = true);

  int get_num_skipped_frames() const {
    return m_num_skipped_frames;
  }

  void dump_info() const;

  std::string get_nalu_type_name(int type);

  bool has_stream_default_duration() const {
    return -1 != m_stream_default_duration;
  }
  int64_t get_stream_default_duration() const {
    assert(-1 != m_stream_default_duration);
    return m_stream_default_duration;
  }

  int64_t duration_for(unsigned int sps, bool field_pic_flag) const;
  int64_t duration_for(slice_info_t const &si) const {
    return duration_for(si.sps, si.field_pic_flag);
  }
  int64_t get_most_often_used_duration() const;

  size_t get_num_field_slices() const;
  size_t get_num_frame_slices() const;

  bool has_par_been_found() const;
  int64_rational_c const &get_par() const;
  std::pair<int64_t, int64_t> const get_display_dimensions(int width = -1, int height = -1) const;

protected:
  bool parse_slice(memory_cptr const &nalu, slice_info_t &si);
  void handle_sps_nalu(memory_cptr const &nalu);
  void handle_pps_nalu(memory_cptr const &nalu);
  void handle_sei_nalu(memory_cptr const &nalu);
  void handle_slice_nalu(memory_cptr const &nalu, uint64_t nalu_pos);
  void cleanup();
  bool flush_decision(slice_info_t &si, slice_info_t &ref);
  void flush_incomplete_frame();
  void flush_unhandled_nalus();
  void add_sps_and_pps_to_extra_data();
  memory_cptr create_nalu_with_size(const memory_cptr &src, bool add_extra_data = false);
  void remove_trailing_zero_bytes(memory_c &memory);
  void init_nalu_names();
  void calculate_frame_order();
  std::vector<int64_t> calculate_provided_timestamps_to_use();
  void calculate_frame_timestamps_and_references();
  void update_frame_stats();
};

}}
