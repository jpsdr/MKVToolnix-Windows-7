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

#include "common/avc_hevc/types.h"
#include "common/avc_hevc/es_parser.h"
#include "common/hevc/types.h"
#include "common/math_fwd.h"
#include "common/dovi_meta.h"

namespace mtx::hevc {

class es_parser_c: public mtx::avc_hevc::es_parser_c {
protected:
  enum class extra_data_position_e {
    pre,
    initial,
    dont_store,
  };

  int m_prev_pic_order_cnt_lsb{}, m_prev_pic_order_cnt_msb{};

  std::vector<memory_cptr> m_extra_data_pre, m_extra_data_initial, m_pending_frame_data;
  std::vector<vps_info_t> m_vps_info_list;
  std::vector<sps_info_t> m_sps_info_list;
  std::vector<pps_info_t> m_pps_info_list;
  user_data_t m_user_data;
  codec_private_t m_codec_private;

  mtx::dovi::dovi_rpu_data_header_t m_dovi_rpu_data_header;

  bool m_normalize_parameter_sets{};

  debugging_option_c m_debug_parameter_sets{"hevc_parser|hevc_parameter_sets"}, m_debug_frame_order{"hevc_parser|hevc_frame_order"};

public:
  es_parser_c();
  ~es_parser_c();

  virtual void flush() override;
  virtual void clear() override;

  virtual void set_configuration_record(memory_cptr const &bytes) override;
  virtual memory_cptr get_configuration_record() const override;

  virtual int get_width() const override {
    assert(!m_sps_info_list.empty());
    return m_sps_info_list.front().get_width();
  }

  virtual int get_height() const override {
    assert(!m_sps_info_list.empty());
    return m_sps_info_list.front().get_height();
  }

  virtual void handle_nalu(memory_cptr const &nalu, uint64_t nalu_pos) override;

  bool headers_parsed() const;

  void normalize_parameter_sets(bool normalize = true);

  void dump_info() const;

  virtual int64_t duration_for(mtx::avc_hevc::slice_info_t const &si) const override;

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
  bool parse_slice(memory_cptr const &nalu, mtx::avc_hevc::slice_info_t &si);
  void handle_nalu_internal(memory_cptr const &nalu, uint64_t nalu_pos);
  void handle_vps_nalu(memory_cptr const &nalu, extra_data_position_e extra_data_position = extra_data_position_e::pre);
  void handle_sps_nalu(memory_cptr const &nalu, extra_data_position_e extra_data_position = extra_data_position_e::pre);
  void handle_pps_nalu(memory_cptr const &nalu, extra_data_position_e extra_data_position = extra_data_position_e::pre);
  void handle_sei_nalu(memory_cptr const &nalu, extra_data_position_e extra_data_position = extra_data_position_e::pre);
  void handle_unspec62_nalu(memory_cptr const &nalu);
  void handle_slice_nalu(memory_cptr const &nalu, uint64_t nalu_pos);
  void flush_incomplete_frame();
  virtual void calculate_frame_order() override;
  void add_parameter_sets_to_extra_data();
  void add_nalu_to_extra_data(memory_cptr const &nalu, extra_data_position_e position = extra_data_position_e::pre);
  void add_nalu_to_pending_frame_data(memory_cptr const &nalu);
  void build_frame_data();

  virtual void init_nalu_names() const override;
};

}                              // namespace mtx::hevc
