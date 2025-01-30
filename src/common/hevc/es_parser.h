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

#include "common/dovi_meta.h"
#include "common/hevc/types.h"
#include "common/math_fwd.h"
#include "common/xyzvc/types.h"
#include "common/xyzvc/es_parser.h"

namespace mtx::hevc {

class es_parser_c: public mtx::xyzvc::es_parser_c {
protected:
  unsigned int m_prev_pic_order_cnt{};
  bool m_first_access_unit_parsed{}, m_first_access_unit_parsing_slices{};

  std::deque<mtx::xyzvc::frame_t> m_frames_awaiting_dovi_el;
  std::vector<vps_info_t> m_vps_info_list;
  std::vector<sps_info_t> m_sps_info_list;
  std::vector<pps_info_t> m_pps_info_list;
  user_data_t m_user_data;
  codec_private_t m_codec_private;

  enum class dovi_el_parsing_state_e {
    pending,
    started,
    finished,
  };
  std::optional<mtx::dovi::dovi_rpu_data_header_t> m_dovi_rpu_data_header;
  std::unique_ptr<es_parser_c> m_dovi_el_config_parser, m_dovi_el_combiner;
  dovi_el_parsing_state_e m_dovi_el_parsing_state{dovi_el_parsing_state_e::pending};
  bool m_dovi_is_enhancement_layer_parser{};

  debugging_option_c m_debug_parameter_sets{"hevc_parser|hevc_parameter_sets"}, m_debug_frame_order{"hevc_parser|hevc_frame_order"}, m_debug_dovi_el_combiner{"hevc_parser|hevc_dovi_layer_combiner"};

public:
  es_parser_c();

  virtual void flush() override;
  virtual void clear() override;

  virtual void set_configuration_record(memory_cptr const &bytes) override;
  virtual memory_cptr get_configuration_record() const override;
  virtual memory_cptr get_dovi_enhancement_layer_configuration_record() const;

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

  virtual int64_t duration_for(mtx::xyzvc::slice_info_t const &si) const override;

  bool has_dovi_rpu_header() const;

  mtx::dovi::dovi_rpu_data_header_t get_dovi_rpu_header() const;

  vui_info_t get_vui_info() const {
    assert(!m_sps_info_list.empty());
    return m_sps_info_list.front().vui;
  }

  void enable_dovi_layer_combiner();

  void add_enhancement_layer_bytes(uint8_t *buf, std::size_t size);
  void add_enhancement_layer_bytes(memory_cptr const &buf);

protected:
  bool parse_slice(memory_cptr const &nalu, mtx::xyzvc::slice_info_t &si);
  void handle_nalu_internal(memory_cptr const &nalu, uint64_t nalu_pos);
  void handle_vps_nalu(memory_cptr const &nalu, extra_data_position_e extra_data_position = extra_data_position_e::pre);
  void handle_sps_nalu(memory_cptr const &nalu, extra_data_position_e extra_data_position = extra_data_position_e::pre);
  void handle_pps_nalu(memory_cptr const &nalu, extra_data_position_e extra_data_position = extra_data_position_e::pre);
  void handle_sei_nalu(memory_cptr const &nalu, extra_data_position_e extra_data_position = extra_data_position_e::pre);
  void handle_unspec62_nalu(memory_cptr const &nalu);
  void handle_unspec63_nalu(memory_cptr const &nalu);
  void handle_slice_nalu(memory_cptr const &nalu, uint64_t nalu_pos);
  void flush_incomplete_frame();
  void determine_if_first_access_unit_parsed();
  void maybe_set_configuration_record_ready();
  virtual void calculate_frame_order() override;
  unsigned int calculate_picture_order_cnt(mtx::xyzvc::frame_t const &frame, sps_info_t const &sps) const;
  virtual bool does_nalu_get_included_in_extra_data(memory_c const &nalu) const override;

  bool add_dovi_combiner_frame_data(mtx::xyzvc::frame_t &frame);

  void cleanup_and_combine_dovi_layers();
  void combine_dovi_layers();

  static void init_nalu_names();
};

}                              // namespace mtx::hevc
