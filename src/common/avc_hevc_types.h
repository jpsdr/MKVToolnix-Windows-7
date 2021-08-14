/** AVC & HEVC video helper functions

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   \author Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx::avc_hevc {

struct slice_info_t {
  // Fields common to AVC & HEVC:
  unsigned char nalu_type{};
  unsigned char slice_type{};
  unsigned char pps_id{};
  unsigned int sps{};
  unsigned int pps{};
  unsigned int pic_order_cnt_lsb{};

  // AVC-specific fields:
  unsigned char nal_ref_idc{};
  unsigned int frame_num{};
  bool field_pic_flag{}, bottom_field_flag{};
  unsigned int idr_pic_id{};
  unsigned int delta_pic_order_cnt_bottom{};
  unsigned int delta_pic_order_cnt[2]{};
  unsigned int first_mb_in_slice{};

  // HEVC-specific fields:
  bool first_slice_segment_in_pic_flag{};
  int temporal_id{};

  void dump() const;
  void clear();
};

}
