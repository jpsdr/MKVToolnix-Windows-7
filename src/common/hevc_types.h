/** HEVC video helper functions

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   \file

*/

#pragma once

#include "common/common_pch.h"

#include "common/math.h"

#define NALU_START_CODE 0x00000001

// VCL NALs
#define HEVC_NALU_TYPE_TRAIL_N        0
#define HEVC_NALU_TYPE_TRAIL_R        1
#define HEVC_NALU_TYPE_TSA_N          2
#define HEVC_NALU_TYPE_TSA_R          3
#define HEVC_NALU_TYPE_STSA_N         4
#define HEVC_NALU_TYPE_STSA_R         5
#define HEVC_NALU_TYPE_RADL_N         6
#define HEVC_NALU_TYPE_RADL_R         7
#define HEVC_NALU_TYPE_RASL_N         8
#define HEVC_NALU_TYPE_RASL_R         9
#define HEVC_NALU_TYPE_RSV_VCL_N10    10
#define HEVC_NALU_TYPE_RSV_VCL_N12    12
#define HEVC_NALU_TYPE_RSV_VCL_N14    14
#define HEVC_NALU_TYPE_RSV_VCL_R11    11
#define HEVC_NALU_TYPE_RSV_VCL_R13    13
#define HEVC_NALU_TYPE_RSV_VCL_R15    15
#define HEVC_NALU_TYPE_BLA_W_LP       16
#define HEVC_NALU_TYPE_BLA_W_RADL     17
#define HEVC_NALU_TYPE_BLA_N_LP       18
#define HEVC_NALU_TYPE_IDR_W_RADL     19
#define HEVC_NALU_TYPE_IDR_N_LP       20
#define HEVC_NALU_TYPE_CRA_NUT        21
#define HEVC_NALU_TYPE_RSV_RAP_VCL22  22
#define HEVC_NALU_TYPE_RSV_RAP_VCL23  23
#define HEVC_NALU_TYPE_RSV_VCL24      24
#define HEVC_NALU_TYPE_RSV_VCL25      25
#define HEVC_NALU_TYPE_RSV_VCL26      26
#define HEVC_NALU_TYPE_RSV_VCL27      27
#define HEVC_NALU_TYPE_RSV_VCL28      28
#define HEVC_NALU_TYPE_RSV_VCL29      29
#define HEVC_NALU_TYPE_RSV_VCL30      30
#define HEVC_NALU_TYPE_RSV_VCL31      31

//Non-VCL NALs
#define HEVC_NALU_TYPE_VIDEO_PARAM    32
#define HEVC_NALU_TYPE_SEQ_PARAM      33
#define HEVC_NALU_TYPE_PIC_PARAM      34
#define HEVC_NALU_TYPE_ACCESS_UNIT    35
#define HEVC_NALU_TYPE_END_OF_SEQ     36
#define HEVC_NALU_TYPE_END_OF_STREAM  37
#define HEVC_NALU_TYPE_FILLER_DATA    38
#define HEVC_NALU_TYPE_PREFIX_SEI     39
#define HEVC_NALU_TYPE_SUFFIX_SEI     40
#define HEVC_NALU_TYPE_RSV_NVCL41     41
#define HEVC_NALU_TYPE_RSV_NVCL42     42
#define HEVC_NALU_TYPE_RSV_NVCL43     43
#define HEVC_NALU_TYPE_RSV_NVCL44     44
#define HEVC_NALU_TYPE_RSV_NVCL45     45
#define HEVC_NALU_TYPE_RSV_NVCL46     46
#define HEVC_NALU_TYPE_RSV_NVCL47     47
#define HEVC_NALU_TYPE_UNSPEC48       48
#define HEVC_NALU_TYPE_UNSPEC49       49
#define HEVC_NALU_TYPE_UNSPEC50       50
#define HEVC_NALU_TYPE_UNSPEC51       51
#define HEVC_NALU_TYPE_UNSPEC52       52
#define HEVC_NALU_TYPE_UNSPEC53       53
#define HEVC_NALU_TYPE_UNSPEC54       54
#define HEVC_NALU_TYPE_UNSPEC55       55
#define HEVC_NALU_TYPE_UNSPEC56       56
#define HEVC_NALU_TYPE_UNSPEC57       57
#define HEVC_NALU_TYPE_UNSPEC58       58
#define HEVC_NALU_TYPE_UNSPEC59       59
#define HEVC_NALU_TYPE_UNSPEC60       60
#define HEVC_NALU_TYPE_UNSPEC61       61
#define HEVC_NALU_TYPE_UNSPEC62       62
#define HEVC_NALU_TYPE_UNSPEC63       63

#define HEVC_SEI_BUFFERING_PERIOD                      0
#define HEVC_SEI_PICTURE_TIMING                        1
#define HEVC_SEI_PAN_SCAN_RECT                         2
#define HEVC_SEI_FILLER_PAYLOAD                        3
#define HEVC_SEI_USER_DATA_REGISTERED_ITU_T_T35        4
#define HEVC_SEI_USER_DATA_UNREGISTERED                5
#define HEVC_SEI_RECOVERY_POINT                        6
#define HEVC_SEI_SCENE_INFO                            9
#define HEVC_SEI_FULL_FRAME_SNAPSHOT                   15
#define HEVC_SEI_PROGRESSIVE_REFINEMENT_SEGMENT_START  16
#define HEVC_SEI_PROGRESSIVE_REFINEMENT_SEGMENT_END    17
#define HEVC_SEI_FILM_GRAIN_CHARACTERISTICS            19
#define HEVC_SEI_POST_FILTER_HINT                      22
#define HEVC_SEI_TONE_MAPPING_INFO                     23
#define HEVC_SEI_FRAME_PACKING                         45
#define HEVC_SEI_DISPLAY_ORIENTATION                   47
#define HEVC_SEI_SOP_DESCRIPTION                       128
#define HEVC_SEI_ACTIVE_PARAMETER_SETS                 129
#define HEVC_SEI_DECODING_UNIT_INFO                    130
#define HEVC_SEI_TEMPORAL_LEVEL0_INDEX                 131
#define HEVC_SEI_DECODED_PICTURE_HASH                  132
#define HEVC_SEI_SCALABLE_NESTING                      133
#define HEVC_SEI_REGION_REFRESH_INFO                   134

#define HEVC_SLICE_TYPE_P   0
#define HEVC_SLICE_TYPE_B   1
#define HEVC_SLICE_TYPE_I   2

#define HEVC_EXTENDED_SAR        0xff
#define HEVC_NUM_PREDEFINED_PARS   17

namespace mtx { namespace hevc {

struct frame_t;
struct par_extraction_t;
struct slice_info_t;

/*
Bytes                                    Bits
      configuration_version               8     The value should be 0 until the format has been finalized. Thereafter is should have the specified value
                                                (probably 1). This allows us to recognize (and ignore) non-standard CodecPrivate
1
      general_profile_space               2     Specifies the context for the interpretation of general_profile_idc and
                                                general_profile_compatibility_flag
      general_tier_flag                   1     Specifies the context for the interpretation of general_level_idc
      general_profile_idc                 5     Defines the profile of the bitstream
1
      general_profile_compatibility_flag  32    Defines profile compatibility, see [2] for interpretation
4
      general_progressive_source_flag     1     Source is progressive, see [2] for interpretation.
      general_interlace_source_flag       1     Source is interlaced, see [2] for interpretation.
      general_nonpacked_constraint_flag   1     If 1 then no frame packing arrangement SEI messages, see [2] for more information
      general_frame_only_constraint_flag  1     If 1 then no fields, see [2] for interpretation
      reserved                            44    Reserved field, value TBD 0
6
      general_level_idc                   8     Defines the level of the bitstream
1
      reserved                            4     Reserved Field, value '1111'b
      min_spatial_segmentation_idc        12    Maximum possible size of distinct coded spatial segmentation regions in the pictures of the CVS
2
      reserved                            6     Reserved Field, value '111111'b
      Parallelism_type                    2     0=unknown, 1=slices, 2=tiles, 3=WPP
1
      reserved                            6     Reserved field, value '111111'b
      chroma_format_idc                   2     See table 6-1, HEVC
1
      reserved                            5     Reserved Field, value '11111'b
      bit_depth_luma_minus8               3     Bit depth luma minus 8
1
      reserved                            5     Reserved Field, value '11111'b
      bit_depth_chroma_minus8             3     Bit depth chroma minus 8
1
      reserved                            16    Reserved Field, value 0
2
      reserved                            2     Reserved Field, value 0
      max_sub_layers                      3     maximum number of temporal sub-layers
      temporal_id_nesting_flag            1     Specifies whether inter prediction is additionally restricted. see [2] for interpretation.
      size_nalu_minus_one                 2     Size of field NALU Length – 1
1
      num_parameter_sets                  8     Number of parameter sets
1
--
23 bytes total
*/

struct codec_private_t {
  unsigned char configuration_version;

  // vps data
  unsigned int profile_space;
  unsigned int tier_flag;
  unsigned int profile_idc;
  unsigned int profile_compatibility_flag;
  unsigned int progressive_source_flag;
  unsigned int interlaced_source_flag;
  unsigned int non_packed_constraint_flag;
  unsigned int frame_only_constraint_flag;
  unsigned int level_idc;

  // sps data
  unsigned int  min_spatial_segmentation_idc;
  unsigned char chroma_format_idc;
  unsigned char bit_depth_luma_minus8;
  unsigned char bit_depth_chroma_minus8;
  unsigned int  max_sub_layers_minus1;
  unsigned int  temporal_id_nesting_flag;

  unsigned char num_parameter_sets;
  unsigned char parallelism_type;

  int vps_data_id;
  int sps_data_id;

  codec_private_t() {
    memset(this, 0, sizeof(*this));

    vps_data_id = -1;
    sps_data_id = -1;
  }
};

struct vps_info_t {
  unsigned int id;

  unsigned int profile_space;
  unsigned int tier_flag;
  unsigned int profile_idc;
  unsigned int profile_compatibility_flag;
  unsigned int progressive_source_flag;
  unsigned int interlaced_source_flag;
  unsigned int non_packed_constraint_flag;
  unsigned int frame_only_constraint_flag;
  unsigned int level_idc;

  unsigned int max_sub_layers_minus1;

  uint32_t checksum;

  vps_info_t() {
    memset(this, 0, sizeof(*this));
  }
};

struct short_term_ref_pic_set_t {
  int inter_ref_pic_set_prediction_flag;
  int delta_idx;
  int delta_rps_sign;
  int abs_delta_rps;

  int delta_poc[16+1];
  int used[16+1];
  int ref_id[16+1];

  int num_ref_id;
  int num_pics;
  int num_negative_pics;
  int num_positive_pics;
};

struct sps_info_t {
  unsigned int id;
  unsigned int vps_id;
  unsigned int max_sub_layers_minus1;
  unsigned int temporal_id_nesting_flag;

  short_term_ref_pic_set_t short_term_ref_pic_sets[64];

  unsigned int chroma_format_idc;
  unsigned int bit_depth_luma_minus8;
  unsigned int bit_depth_chroma_minus8;
  unsigned int separate_colour_plane_flag;
  unsigned int log2_min_luma_coding_block_size_minus3;
  unsigned int log2_diff_max_min_luma_coding_block_size;
  unsigned int log2_max_pic_order_cnt_lsb;

  bool conformance_window_flag;
  unsigned int conf_win_left_offset, conf_win_right_offset, conf_win_top_offset, conf_win_bottom_offset;

  // vui:
  bool vui_present, ar_found;
  unsigned int par_num, par_den;
  unsigned int min_spatial_segmentation_idc;

  // timing_info:
  bool timing_info_present;
  unsigned int num_units_in_tick, time_scale;

  unsigned int width, height;

  unsigned int vps;

  uint32_t checksum;

  sps_info_t() {
    memset(this, 0, sizeof(*this));
  }

  void dump();

  bool timing_info_valid() const;
  int64_t default_duration() const;
};

struct pps_info_t {
  unsigned id;
  unsigned sps_id;

  bool dependent_slice_segments_enabled_flag;
  bool output_flag_present_flag;
  unsigned int num_extra_slice_header_bits;

  uint32_t checksum;

  pps_info_t() {
    memset(this, 0, sizeof(*this));
  }

  void dump();
};

class es_parser_c;

using user_data_t = std::map< std::vector<unsigned char>, std::vector<unsigned char> >;
using es_parser_cptr = std::shared_ptr<es_parser_c>;

}}                              // namespace mtx::hevc
