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

#include "common/avc/util.h"
#include "common/math_fwd.h"
#include "common/xyzvc/types.h"
#include "common/xyzvc/es_parser.h"

namespace mtx::avc {

class es_parser_c: public mtx::xyzvc::es_parser_c {
protected:
  bool m_fix_bitstream_frame_rate{};

  std::optional<bool> m_current_key_frame_bottom_field;

  std::vector<sps_info_t> m_sps_info_list;
  std::vector<pps_info_t> m_pps_info_list;

  bool m_all_i_slices_are_key_frames{}, m_have_incomplete_frame{};

  debugging_option_c m_debug_sps_pps_changes{"avc_parser|avc_sps_pps_changes"}, m_debug_errors{"avc_parser|avc_errors"};

public:
  es_parser_c();

  bool has_timing_info() const {
    return !m_sps_info_list.empty() && m_sps_info_list[0].timing_info_valid();
  }

  timing_info_t get_timing_info() const {
    if (!has_timing_info())
      throw std::out_of_range{"no timing information present"};
    return m_sps_info_list[0].timing_info;
  }

  void set_fix_bitstream_frame_rate(bool fix) {
    m_fix_bitstream_frame_rate = fix;
  }

  virtual void flush() override;
  virtual void clear() override;

  virtual void set_configuration_record(memory_cptr const &bytes) override;
  virtual memory_cptr get_configuration_record() const override;

  virtual int get_width() const override {
    assert(!m_sps_info_list.empty());
    return m_sps_info_list.begin()->width;
  }

  virtual int get_height() const override {
    assert(!m_sps_info_list.empty());
    return m_sps_info_list.begin()->height;
  }

  virtual void handle_nalu(memory_cptr const &nalu, uint64_t nalu_pos) override;

  bool headers_parsed() const;

  virtual int64_t duration_for(mtx::xyzvc::slice_info_t const &si) const override;

  bool parse_slice(memory_cptr const &nalu, mtx::xyzvc::slice_info_t &si);
  void handle_sps_nalu(memory_cptr const &nalu);
  void handle_pps_nalu(memory_cptr const &nalu);
  void handle_sei_nalu(memory_cptr const &nalu);
  void handle_slice_nalu(memory_cptr const &nalu, uint64_t nalu_pos);

protected:
  bool flush_decision(mtx::xyzvc::slice_info_t &si, mtx::xyzvc::slice_info_t &ref);
  void flush_incomplete_frame();
  void add_sps_and_pps_to_extra_data();
  memory_cptr create_nalu_with_size(const memory_cptr &src, bool add_extra_data = false);

  int64_t duration_for_impl(unsigned int sps, bool field_pic_flag) const;
  virtual void calculate_frame_order() override;
  virtual bool does_nalu_get_included_in_extra_data(memory_c const &nalu) const override;

  static void init_nalu_names();
};

}
