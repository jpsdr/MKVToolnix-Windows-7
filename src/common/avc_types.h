/** MPEG-4 p10 video helper functions

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

#include "common/math.h"

#define NALU_START_CODE 0x00000001

#define NALU_TYPE_NON_IDR_SLICE  0x01
#define NALU_TYPE_DP_A_SLICE     0x02
#define NALU_TYPE_DP_B_SLICE     0x03
#define NALU_TYPE_DP_C_SLICE     0x04
#define NALU_TYPE_IDR_SLICE      0x05
#define NALU_TYPE_SEI            0x06
#define NALU_TYPE_SEQ_PARAM      0x07
#define NALU_TYPE_PIC_PARAM      0x08
#define NALU_TYPE_ACCESS_UNIT    0x09
#define NALU_TYPE_END_OF_SEQ     0x0a
#define NALU_TYPE_END_OF_STREAM  0x0b
#define NALU_TYPE_FILLER_DATA    0x0c

#define AVC_SLICE_TYPE_P   0
#define AVC_SLICE_TYPE_B   1
#define AVC_SLICE_TYPE_I   2
#define AVC_SLICE_TYPE_SP  3
#define AVC_SLICE_TYPE_SI  4
#define AVC_SLICE_TYPE2_P  5
#define AVC_SLICE_TYPE2_B  6
#define AVC_SLICE_TYPE2_I  7
#define AVC_SLICE_TYPE2_SP 8
#define AVC_SLICE_TYPE2_SI 9

#define AVC_EXTENDED_SAR        0xff
#define AVC_NUM_PREDEFINED_PARS   17

namespace mtx { namespace avc {

struct timing_info_t {
  unsigned int num_units_in_tick, time_scale;
  bool is_present, fixed_frame_rate;

  int64_t default_duration() const;
  bool is_valid() const;
};

struct sps_info_t {
  unsigned int id;

  unsigned int profile_idc;
  unsigned int profile_compat;
  unsigned int level_idc;
  unsigned int chroma_format_idc;
  unsigned int log2_max_frame_num;
  unsigned int pic_order_cnt_type;
  unsigned int log2_max_pic_order_cnt_lsb;
  unsigned int offset_for_non_ref_pic;
  unsigned int offset_for_top_to_bottom_field;
  unsigned int num_ref_frames_in_pic_order_cnt_cycle;
  bool delta_pic_order_always_zero_flag;
  bool frame_mbs_only;

  // vui:
  bool vui_present, ar_found;
  unsigned int par_num, par_den;

  // timing_info:
  timing_info_t timing_info;

  unsigned int crop_left, crop_top, crop_right, crop_bottom;
  unsigned int width, height;

  uint32_t checksum;

  sps_info_t() {
    memset(this, 0, sizeof(*this));
  }

  void dump();

  bool timing_info_valid() const;
};

struct pps_info_t {
  unsigned id;
  unsigned sps_id;

  bool pic_order_present;

  uint32_t checksum;

  pps_info_t() {
    memset(this, 0, sizeof(*this));
  }

  void dump();
};

struct slice_info_t {
  unsigned char nalu_type;
  unsigned char nal_ref_idc;
  unsigned char type;
  unsigned char pps_id;
  unsigned int frame_num;
  bool field_pic_flag, bottom_field_flag;
  unsigned int idr_pic_id;
  unsigned int pic_order_cnt_lsb;
  unsigned int delta_pic_order_cnt_bottom;
  unsigned int delta_pic_order_cnt[2];
  unsigned int first_mb_in_slice;

  unsigned int sps;
  unsigned int pps;

  slice_info_t() {
    clear();
  }

  void dump() const;
  void clear() {
    memset(this, 0, sizeof(*this));
  }
};

struct par_extraction_t {
  memory_cptr new_avcc;
  unsigned int numerator, denominator;
  bool successful;

  bool is_valid() const;
};

struct frame_t;
class es_parser_c;
using es_parser_cptr = std::shared_ptr<es_parser_c>;

}}
