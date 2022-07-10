/** MPEG video helper functions (MPEG 1, 2 and 4)

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   \file

*/

#include "common/common_pch.h"

#include <cmath>
#include <unordered_map>

#include "common/bit_reader.h"
#include "common/bit_writer.h"
#include "common/byte_buffer.h"
#include "common/avc_hevc/types.h"
#include "common/checksums/base.h"
#include "common/endian.h"
#include "common/hacks.h"
#include "common/mm_io_x.h"
#include "common/mm_mem_io.h"
#include "common/mpeg.h"
#include "common/hevc/util.h"
#include "common/hevc/hevcc.h"

namespace mtx::hevc {

static const struct {
  int numerator, denominator;
} s_predefined_pars[NUM_PREDEFINED_PARS] = {
  {   0,  0 },
  {   1,  1 },
  {  12, 11 },
  {  10, 11 },
  {  16, 11 },
  {  40, 33 },
  {  24, 11 },
  {  20, 11 },
  {  32, 11 },
  {  80, 33 },
  {  18, 11 },
  {  15, 11 },
  {  64, 33 },
  { 160, 99 },
  {   4,  3 },
  {   3,  2 },
  {   2,  1 },
};

bool
par_extraction_t::is_valid()
  const {
  return successful && numerator && denominator;
}

static void
profile_tier_copy(mtx::bits::reader_c &r,
                  mtx::bits::writer_c &w,
                  vps_info_t &vps,
                  unsigned int maxNumSubLayersMinus1) {
  unsigned int i;
  std::vector<bool> sub_layer_profile_present_flag, sub_layer_level_present_flag;

  vps.profile_space = w.copy_bits(2, r);
  vps.tier_flag = w.copy_bits(1, r);
  vps.profile_idc = w.copy_bits(5, r);  // general_profile_idc
  vps.profile_compatibility_flag = w.copy_bits(32, r);
  vps.progressive_source_flag = w.copy_bits(1, r);
  vps.interlaced_source_flag = w.copy_bits(1, r);
  vps.non_packed_constraint_flag = w.copy_bits(1, r);
  vps.frame_only_constraint_flag = w.copy_bits(1, r);
  w.copy_bits(44, r);     // general_reserved_zero_44bits
  vps.level_idc = w.copy_bits(8, r);    // general_level_idc

  for (i = 0; i < maxNumSubLayersMinus1; i++) {
    sub_layer_profile_present_flag.push_back(w.copy_bits(1, r)); // sub_layer_profile_present_flag[i]
    sub_layer_level_present_flag.push_back(w.copy_bits(1, r));   // sub_layer_level_present_flag[i]
  }

  if (maxNumSubLayersMinus1 > 0)
    for (i = maxNumSubLayersMinus1; i < 8; i++)
      w.copy_bits(2, r);  // reserved_zero_2bits

  for (i = 0; i < maxNumSubLayersMinus1; i++) {
    if (sub_layer_profile_present_flag[i]) {
      w.copy_bits(2+1+5, r);  // sub_layer_profile_space[i], sub_layer_tier_flag[i], sub_layer_profile_idc[i]
      w.copy_bits(32, r);     // sub_layer_profile_compatibility_flag[i][]
      w.copy_bits(4, r);      // sub_layer_progressive_source_flag[i], sub_layer_interlaced_source_flag[i], sub_layer_non_packed_constraint_flag[i], sub_layer_frame_only_constraint_flag[i]
      w.copy_bits(44, r);     // sub_layer_reserved_zero_44bits[i]
    }
    if (sub_layer_level_present_flag[i]) {
      w.copy_bits(8, r);      // sub_layer_level_idc[i]
    }
  }
}

static void
sub_layer_hrd_parameters_copy(mtx::bits::reader_c &r,
                              mtx::bits::writer_c &w,
                              unsigned int CpbCnt,
                              bool sub_pic_cpb_params_present_flag) {
  unsigned int i;

  for (i = 0; i <= CpbCnt; i++) {
    w.copy_unsigned_golomb(r); // bit_rate_value_minus1[i]
    w.copy_unsigned_golomb(r); // cpb_size_value_minus1[i]

    if (sub_pic_cpb_params_present_flag) {
      w.copy_unsigned_golomb(r); // cpb_size_du_value_minus1[i]
      w.copy_unsigned_golomb(r); // bit_rate_du_value_minus1[i]
    }

    w.copy_bits(1, r); // cbr_flag[i]
  }
}

static void
hrd_parameters_copy(mtx::bits::reader_c &r,
                    mtx::bits::writer_c &w,
                    bool commonInfPresentFlag,
                    unsigned int maxNumSubLayersMinus1) {
  unsigned int i;

  bool nal_hrd_parameters_present_flag = false;
  bool vcl_hrd_parameters_present_flag = false;
  bool sub_pic_cpb_params_present_flag = false;

  if (commonInfPresentFlag) {
    nal_hrd_parameters_present_flag = w.copy_bits(1, r); // nal_hrd_parameters_present_flag
    vcl_hrd_parameters_present_flag = w.copy_bits(1, r); // vcl_hrd_parameters_present_flag

    if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag) {
      sub_pic_cpb_params_present_flag = w.copy_bits(1, r); // sub_pic_cpb_params_present_flag
      if (sub_pic_cpb_params_present_flag) {
        w.copy_bits(8, r);  // tick_divisor_minus2
        w.copy_bits(5, r);  // du_cpb_removal_delay_increment_length_minus1
        w.copy_bits(1, r);  // sub_pic_cpb_params_in_pic_timing_sei_flag
        w.copy_bits(5, r);  // dpb_output_delay_du_length_minus1
      }

      w.copy_bits(4+4, r);  // bit_rate_scale, cpb_size_scale

      if (sub_pic_cpb_params_present_flag)
        w.copy_bits(4, r);  // cpb_size_du_scale

      w.copy_bits(5, r);  // initial_cpb_removal_delay_length_minus1
      w.copy_bits(5, r);  // au_cpb_removal_delay_length_minus1
      w.copy_bits(5, r);  // dpb_output_delay_length_minus1
    }
  }

  for (i = 0; i <= maxNumSubLayersMinus1; i++) {
    bool fixed_pic_rate_general_flag = w.copy_bits(1, r); // fixed_pic_rate_general_flag[i]
    bool fixed_pic_rate_within_cvs_flag = false;
    bool low_delay_hrd_flag = false;
    unsigned int CpbCnt = 0;

    if (!fixed_pic_rate_general_flag)
      fixed_pic_rate_within_cvs_flag = w.copy_bits(1, r); // fixed_pic_rate_within_cvs_flag[i]
    else {
      // E.2.2 - When fixed_pic_rate_general_flag[i] is equal to 1,
      // the value of fixed_pic_rate_within_cvs_flag[i] is inferred to be equal to 1.
      fixed_pic_rate_within_cvs_flag = true;
    }
    if (fixed_pic_rate_within_cvs_flag)
      w.copy_unsigned_golomb(r);                                       // elemental_duration_in_tc_minus1[i]
    else
      low_delay_hrd_flag = w.copy_bits(1, r);             // low_delay_hrd_flag[i]

    if (!low_delay_hrd_flag)
      CpbCnt = w.copy_unsigned_golomb(r);                              // cpb_cnt_minus1[i]

    if (nal_hrd_parameters_present_flag)
      sub_layer_hrd_parameters_copy(r, w, CpbCnt, sub_pic_cpb_params_present_flag);

    if (vcl_hrd_parameters_present_flag)
      sub_layer_hrd_parameters_copy(r, w, CpbCnt, sub_pic_cpb_params_present_flag);
  }
}

static void
scaling_list_data_copy(mtx::bits::reader_c &r,
                       mtx::bits::writer_c &w) {
  unsigned int i;
  unsigned int sizeId;
  for (sizeId = 0; sizeId < 4; sizeId++) {
    unsigned int matrixId;
    for (matrixId = 0; matrixId < ((sizeId == 3) ? 2 : 6); matrixId++) {
      if (w.copy_bits(1, r) == 0) {  // scaling_list_pred_mode_flag[sizeId][matrixId]
        w.copy_unsigned_golomb(r); // scaling_list_pred_matrix_id_delta[sizeId][matrixId]
      } else {
        unsigned int coefNum = std::min(64, (1 << (4 + (sizeId << 1))));

        if (sizeId > 1) {
          w.copy_signed_golomb(r);  // scaling_list_dc_coef_minus8[sizeId - 2][matrixId]
        }

        for (i = 0; i < coefNum; i++) {
          w.copy_signed_golomb(r);  // scaling_list_delta_coef
        }
      }
    }
  }
}

static void
short_term_ref_pic_set_copy(mtx::bits::reader_c &r,
                            mtx::bits::writer_c &w,
                            short_term_ref_pic_set_t *short_term_ref_pic_sets,
                            unsigned int idx_rps,
                            unsigned int num_short_term_ref_pic_sets) {
  if (idx_rps >= 64)
    throw false;

  short_term_ref_pic_set_t* ref_st_rp_set;
  short_term_ref_pic_set_t* cur_st_rp_set = short_term_ref_pic_sets + idx_rps;
  unsigned int inter_rps_pred_flag = cur_st_rp_set->inter_ref_pic_set_prediction_flag = 0;

  if (idx_rps > 0)
    inter_rps_pred_flag = cur_st_rp_set->inter_ref_pic_set_prediction_flag = w.copy_bits(1, r); // inter_ref_pic_set_prediction_flag

  if (inter_rps_pred_flag) {
    int ref_idx;
    int delta_rps;
    int i;
    int k = 0;
    int k0 = 0;
    int k1 = 0;
    int code = 0;

    if (idx_rps == num_short_term_ref_pic_sets)
      code = w.copy_unsigned_golomb(r); // delta_idx_minus1

    cur_st_rp_set->delta_idx = code + 1;
    ref_idx                  = idx_rps - 1 - code;

    if (ref_idx >= 64)
      throw false;

    ref_st_rp_set = short_term_ref_pic_sets + ref_idx;

    cur_st_rp_set->delta_rps_sign = w.copy_bits(1, r);  // delta_rps_sign
    cur_st_rp_set->delta_idx = w.copy_unsigned_golomb(r) + 1;  // abs_delta_rps_minus1

    delta_rps = (1 - cur_st_rp_set->delta_rps_sign*2) * cur_st_rp_set->delta_idx;

    // num_pics is used for indexing delta_poc[], user[] and ref_id[],
    // each of which only has 17 entries.
    if ((0 > cur_st_rp_set->num_pics) || (16 < cur_st_rp_set->num_pics))
      throw false;

    for (i = 0; i <= ref_st_rp_set->num_pics; i++) {
      int ref_id = w.copy_bits(1, r); // used_by_curr_pic_flag
      if (ref_id == 0) {
        int bit = w.copy_bits(1, r);  // use_delta_flag
        ref_id = bit*2;
      }

      if (ref_id != 0) {
        int delta_POC = delta_rps + ((i < ref_st_rp_set->num_pics) ? ref_st_rp_set->delta_poc[i] : 0);
        cur_st_rp_set->delta_poc[k] = delta_POC;
        cur_st_rp_set->used[k] = (ref_id == 1) ? 1 : 0;

        k0 += delta_POC < 0;
        k1 += delta_POC >= 0;
        k++;
      }

      cur_st_rp_set->ref_id[i] = ref_id;
    }

    cur_st_rp_set->num_ref_id = ref_st_rp_set->num_pics + 1;
    cur_st_rp_set->num_pics = k;
    cur_st_rp_set->num_negative_pics = k0;
    cur_st_rp_set->num_positive_pics = k1;

    // Sort in increasing order (smallest first)
    for (i = 1; i < cur_st_rp_set->num_pics; i++ ) {
      int delta_POC = cur_st_rp_set->delta_poc[i];
      int used = cur_st_rp_set->used[i];
      for (k = i - 1; k >= 0; k--) {
        int temp = cur_st_rp_set->delta_poc[k];

        if (delta_POC < temp) {
          cur_st_rp_set->delta_poc[k + 1] = temp;
          cur_st_rp_set->used[k + 1] = cur_st_rp_set->used[k];
          cur_st_rp_set->delta_poc[k] = delta_POC;
          cur_st_rp_set->used[k] = used;
        }
      }
    }

    // Flip the negative values to largest first
    for (i = 0, k = cur_st_rp_set->num_negative_pics - 1; i < cur_st_rp_set->num_negative_pics >> 1; i++, k--) {
      int delta_POC = cur_st_rp_set->delta_poc[i];
      int used = cur_st_rp_set->used[i];
      cur_st_rp_set->delta_poc[i] = cur_st_rp_set->delta_poc[k];
      cur_st_rp_set->used[i] = cur_st_rp_set->used[k];
      cur_st_rp_set->delta_poc[k] = delta_POC;
      cur_st_rp_set->used[k] = used;
    }
  } else {
    int prev = 0;
    int poc;
    int i;

    cur_st_rp_set->num_negative_pics = w.copy_unsigned_golomb(r);  // num_negative_pics
    cur_st_rp_set->num_positive_pics = w.copy_unsigned_golomb(r);  // num_positive_pics

    // num_positive_pics and num_negative_pics are used for indexing
    // delta_poc[], user[] and ref_id[]. The second loop is counting
    // from num_negative_pics to num_pics; therefore the former must
    // not be bigger than the latter.
    if (    (cur_st_rp_set->num_positive_pics < 0) || (cur_st_rp_set->num_positive_pics > 16)
         || (cur_st_rp_set->num_negative_pics < 0) || (cur_st_rp_set->num_negative_pics > 16)
         || ((cur_st_rp_set->num_positive_pics + cur_st_rp_set->num_negative_pics) > 16))
      throw false;

    for (i = 0; i < cur_st_rp_set->num_negative_pics; i++) {
      int code = w.copy_unsigned_golomb(r); // delta_poc_s0_minus1
      poc = prev - code - 1;
      prev = poc;
      cur_st_rp_set->delta_poc[i] = poc;
      cur_st_rp_set->used[i] = w.copy_bits(1, r); // used_by_curr_pic_s0_flag
    }

    prev = 0;
    cur_st_rp_set->num_pics = cur_st_rp_set->num_negative_pics + cur_st_rp_set->num_positive_pics;
    for (i = cur_st_rp_set->num_negative_pics; i < cur_st_rp_set->num_pics; i++) {
      int code = w.copy_unsigned_golomb(r); // delta_poc_s1_minus1
      poc = prev + code + 1;
      prev = poc;
      cur_st_rp_set->delta_poc[i] = poc;
      cur_st_rp_set->used[i] = w.copy_bits(1, r); // used_by_curr_pic_s1_flag
    }
  }
}

static void
vui_parameters_copy(mtx::bits::reader_c &r,
                    mtx::bits::writer_c &w,
                    sps_info_t &sps,
                    bool keep_ar_info,
                    unsigned int max_sub_layers_minus1) {
  if (r.get_bit() == 1) {                   // aspect_ratio_info_present_flag
    unsigned int ar_type = r.get_bits(8);   // aspect_ratio_idc

    if (keep_ar_info) {
      w.put_bit(true);                      // aspect_ratio_info_present_flag
      w.put_bits(8, ar_type);               // aspect_ratio_idc
    } else
      w.put_bit(false);                     // aspect_ratio_info_present_flag

    sps.ar_found = true;

    if (EXTENDED_SAR == ar_type) {
      sps.par_num = r.get_bits(16); // sar_width
      sps.par_den = r.get_bits(16); // sar_height

      if (keep_ar_info &&
          0xFF == ar_type) {
        w.put_bits(16, sps.par_num);
        w.put_bits(16, sps.par_den);
      }
    } else if (NUM_PREDEFINED_PARS >= ar_type) {
      sps.par_num = s_predefined_pars[ar_type].numerator;
      sps.par_den = s_predefined_pars[ar_type].denominator;
    }

  } else {
    sps.ar_found = false;
    w.put_bit(false);
  }

  // copy the rest
  if (w.copy_bits(1, r) == 1)   // overscan_info_present_flag
    w.copy_bits(1, r);          // overscan_appropriate_flag
  if (w.copy_bits(1, r) == 1) { // video_signal_type_present_flag
    w.copy_bits(3, r);          // video_format

    sps.vui.video_full_range_flag = w.copy_bits(1, r);      // video_full_range_flag
    if (w.copy_bits(1, r) == 1) {                           // color_desc_present_flag
      sps.vui.color_primaries          = w.copy_bits(8, r); // color_primaries
      sps.vui.transfer_characteristics = w.copy_bits(8, r); // transfer_characteristics
      sps.vui.matrix_coefficients      = w.copy_bits(8, r); // matrix_coefficients
    }
  }
  if (w.copy_bits(1, r) == 1) { // chroma_loc_info_present_flag
    sps.vui.chroma_sample_loc_type_top_field    = w.copy_unsigned_golomb(r); // chroma_sample_loc_type_top_field
    sps.vui.chroma_sample_loc_type_bottom_field = w.copy_unsigned_golomb(r); // chroma_sample_loc_type_bottom_field
  }
  w.copy_bits(1, r);            // neutral_chroma_indication_flag
  sps.field_seq_flag = w.copy_bits(1, r);
  w.copy_bits(1, r);            // frame_field_info_present_flag

  if (   (r.get_remaining_bits() >= 68)
      && (r.peek_bits(21) == 0x100000))
    w.put_bit(false);                // invalid default display window, signal no default_display_window_flag
  else if (w.copy_bits(1, r) == 1) { // default_display_window_flag
    w.copy_unsigned_golomb(r);               // def_disp_win_left_offset
    w.copy_unsigned_golomb(r);               // def_disp_win_right_offset
    w.copy_unsigned_golomb(r);               // def_disp_win_top_offset
    w.copy_unsigned_golomb(r);               // def_disp_win_bottom_offset
  }
  sps.timing_info_present = w.copy_bits(1, r); // vui_timing_info_present_flag
  if (sps.timing_info_present) {
    sps.num_units_in_tick = w.copy_bits(32, r); // vui_num_units_in_tick
    sps.time_scale        = w.copy_bits(32, r); // vui_time_scale
    if (w.copy_bits(1, r) == 1) { // vui_poc_proportional_to_timing_flag
      w.copy_unsigned_golomb(r); // vui_num_ticks_poc_diff_one_minus1
    }
    if (w.copy_bits(1, r) == 1) { // vui_hrd_parameters_present_flag
      hrd_parameters_copy(r, w, true, max_sub_layers_minus1); // hrd_parameters
    }
  }

  if (w.copy_bits(1, r) == 1) { // bitstream_restriction_flag
    w.copy_bits(3, r);          // tiles_fixed_structure_flag, motion_vectors_over_pic_boundaries_flag, restricted_ref_pic_lists_flag
    w.copy_unsigned_golomb(r);  // min_spatial_segmentation_idc
    w.copy_unsigned_golomb(r);  // max_bytes_per_pic_denom
    w.copy_unsigned_golomb(r);  // max_bits_per_min_cu_denom
    w.copy_unsigned_golomb(r);  // log2_max_mv_length_horizontal
    w.copy_unsigned_golomb(r);  // log2_max_mv_length_vertical
  }
}

void
sps_info_t::dump() {
  mxinfo(fmt::format("sps_info dump:\n"
                     "  id:                                    {0}\n"
                     "  log2_max_pic_order_cnt_lsb:            {1}\n"
                     "  vui_present:                           {2}\n"
                     "  ar_found:                              {3}\n"
                     "  par_num:                               {4}\n"
                     "  par_den:                               {5}\n"
                     "  timing_info_present:                   {6}\n"
                     "  num_units_in_tick:                     {7}\n"
                     "  time_scale:                            {8}\n"
                     "  width:                                 {9}\n"
                     "  height:                                {10}\n"
                     "  checksum:                              {11:08x}\n",
                     id,
                     log2_max_pic_order_cnt_lsb,
                     vui_present,
                     ar_found,
                     par_num,
                     par_den,
                     timing_info_present,
                     num_units_in_tick,
                     time_scale,
                     width,
                     height,
                     checksum));
}

bool
sps_info_t::timing_info_valid()
  const {
  return timing_info_present
    && (0 != num_units_in_tick)
    && (0 != time_scale);
}

int64_t
sps_info_t::default_duration()
  const {
  return 1000000000ll * num_units_in_tick / time_scale;
}

void
sps_info_t::clear() {
  *this = sps_info_t{};
}

void
pps_info_t::clear() {
  *this = pps_info_t{};
}

void
pps_info_t::dump() {
  mxinfo(fmt::format("pps_info dump:\n"
                     "id: {0}\n"
                     "sps_id: {1}\n"
                     "checksum: {2:08x}\n",
                     id,
                     sps_id,
                     checksum));
}

void
vps_info_t::clear() {
  *this = vps_info_t{};
}

static bool
parse_vps_internal(memory_cptr const &buffer,
                   vps_info_t &vps) {
  auto size = buffer->get_size();
  mtx::bits::reader_c r(buffer->get_buffer(), size);
  mtx::bits::writer_c w{};
  unsigned int i, j;

  vps.clear();

  w.copy_bits(1, r);            // forbidden_zero_bit
  if (w.copy_bits(6, r) != NALU_TYPE_VIDEO_PARAM)  // nal_unit_type
    return false;
  w.copy_bits(6, r);            // nuh_reserved_zero_6bits
  w.copy_bits(3, r);            // nuh_temporal_id_plus1

  vps.id = w.copy_bits(4, r);                       // vps_video_parameter_set_id
  w.copy_bits(2+6, r);                              // vps_reserved_three_2bits, vps_reserved_zero_6bits
  vps.max_sub_layers_minus1 = w.copy_bits(3, r);    // vps_max_sub_layers_minus1
  w.copy_bits(1+16, r);                             // vps_temporal_id_nesting_flag, vps_reserved_0xffff_16bits

  // At this point we are at newvps + 6 bytes, profile_tier_level follows
  profile_tier_copy(r, w, vps, vps.max_sub_layers_minus1);  // profile_tier_level(vps_max_sub_layers_minus1)

  bool vps_sub_layer_ordering_info_present_flag = w.copy_bits(1, r);  // vps_sub_layer_ordering_info_present_flag
  for (i = (vps_sub_layer_ordering_info_present_flag ? 0 : vps.max_sub_layers_minus1); i <= vps.max_sub_layers_minus1; i++) {
    w.copy_unsigned_golomb(r); // vps_max_dec_pic_buffering_minus1[i]
    w.copy_unsigned_golomb(r); // vps_max_num_reorder_pics[i]
    w.copy_unsigned_golomb(r); // vps_max_latency_increase[i]
  }

  unsigned int vps_max_nuh_reserved_zero_layer_id = w.copy_bits(6, r);  // vps_max_nuh_reserved_zero_layer_id
  bool vps_num_op_sets_minus1 = w.copy_unsigned_golomb(r);       // vps_num_op_sets_minus1
  for (i = 1; i <= vps_num_op_sets_minus1; i++) {
    for (j = 0; j <= vps_max_nuh_reserved_zero_layer_id; j++) { // operation_point_set(i)
      w.copy_bits(1, r);  // layer_id_included_flag
    }
  }

  if (w.copy_bits(1, r) == 1) { // vps_timing_info_present_flag
    w.copy_bits(32, r);         // vps_num_units_in_tick
    w.copy_bits(32, r);         // vps_time_scale
    if (w.copy_bits(1, r) == 1)  // vps_poc_proportional_to_timing_flag
      w.copy_unsigned_golomb(r);             // vps_num_ticks_poc_diff_one_minus1
    unsigned int vps_num_hrd_parameters = w.copy_unsigned_golomb(r); // vps_num_hrd_parameters
    for (i = 0; i < vps_num_hrd_parameters; i++) {
      bool cprms_present_flag = true; // 7.4.3.1 - cprms_present_flag[0] is inferred to be equal to 1.
      w.copy_unsigned_golomb(r);             // hrd_op_set_idx[i]
      if (i > 0)
        cprms_present_flag = w.copy_bits(1, r); // cprms_present_flag[i]
      hrd_parameters_copy(r, w, cprms_present_flag, vps.max_sub_layers_minus1);
    }
  }

  if (w.copy_bits(1, r) == 1)    // vps_extension_flag
    while (r.get_remaining_bits())
      w.copy_bits(1, r);        // vps_extension_data_flag

  w.put_bit(true);
  w.byte_align();

  // Given we don't change the NALU while writing to w,
  // then we don't need to replace buffer with the bits we've written into w.
  // Leaving this code as reference if we ever do change the NALU while writing to w.
  //buffer = w.get_buffer();

  vps.checksum = mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, *buffer);

  return true;
}

bool
parse_vps(memory_cptr const &buffer,
          vps_info_t &vps) {
  try {
    return parse_vps_internal(buffer, vps);
  } catch (mtx::mm_io::exception &) {
  }

  return false;
}

static memory_cptr
parse_sps_internal(memory_cptr const &buffer,
                   sps_info_t &sps,
                   std::vector<vps_info_t> &vps_info_list,
                   bool keep_ar_info) {
  auto size = buffer->get_size();
  mtx::bits::reader_c r(buffer->get_buffer(), size);
  mtx::bits::writer_c w{};
  unsigned int i;

  keep_ar_info = !mtx::hacks::is_engaged(mtx::hacks::REMOVE_BITSTREAM_AR_INFO);

  sps.clear();
  sps.par_num  = 1;
  sps.par_den  = 1;
  sps.ar_found = false;

  w.copy_bits(1, r);            // forbidden_zero_bit
  if (w.copy_bits(6, r) != NALU_TYPE_SEQ_PARAM)  // nal_unit_type
    return {};
  w.copy_bits(6, r);            // nuh_reserved_zero_6bits
  w.copy_bits(3, r);            // nuh_temporal_id_plus1

  sps.vps_id                   = w.copy_bits(4, r); // sps_video_parameter_set_id
  sps.max_sub_layers_minus1    = w.copy_bits(3, r); // sps_max_sub_layers_minus1
  sps.temporal_id_nesting_flag = w.copy_bits(1, r); // sps_temporal_id_nesting_flag

  size_t vps_idx;
  for (vps_idx = 0; vps_info_list.size() > vps_idx; ++vps_idx)
    if (vps_info_list[vps_idx].id == sps.vps_id)
      break;
  if (vps_info_list.size() == vps_idx)
    return {};

  sps.vps = vps_idx;

  auto &vps = vps_info_list[vps_idx];

  profile_tier_copy(r, w, vps, sps.max_sub_layers_minus1);  // profile_tier_level(sps_max_sub_layers_minus1)

  sps.id = w.copy_unsigned_golomb(r);  // sps_seq_parameter_set_id

  if ((sps.chroma_format_idc = w.copy_unsigned_golomb(r)) == 3) // chroma_format_idc
    sps.separate_color_plane_flag = w.copy_bits(1, r);     // separate_color_plane_flag

  sps.width                   = w.copy_unsigned_golomb(r); // pic_width_in_luma_samples
  sps.height                  = w.copy_unsigned_golomb(r); // pic_height_in_luma_samples
  sps.conformance_window_flag = w.copy_bits(1, r);

  if (sps.conformance_window_flag) {
    sps.conf_win_left_offset   = w.copy_unsigned_golomb(r); // conf_win_left_offset
    sps.conf_win_right_offset  = w.copy_unsigned_golomb(r); // conf_win_right_offset
    sps.conf_win_top_offset    = w.copy_unsigned_golomb(r); // conf_win_top_offset
    sps.conf_win_bottom_offset = w.copy_unsigned_golomb(r); // conf_win_bottom_offset
  }

  sps.bit_depth_luma_minus8                     = w.copy_unsigned_golomb(r);     // bit_depth_luma_minus8
  sps.bit_depth_chroma_minus8                   = w.copy_unsigned_golomb(r);     // bit_depth_chroma_minus8
  sps.log2_max_pic_order_cnt_lsb                = w.copy_unsigned_golomb(r) + 4; // log2_max_pic_order_cnt_lsb_minus4
  auto sps_sub_layer_ordering_info_present_flag = w.copy_bits(1, r);             // sps_sub_layer_ordering_info_present_flag

  for (i = (sps_sub_layer_ordering_info_present_flag ? 0 : sps.max_sub_layers_minus1); i <= sps.max_sub_layers_minus1; i++) {
    w.copy_unsigned_golomb(r); // sps_max_dec_pic_buffering_minus1[i]
    w.copy_unsigned_golomb(r); // sps_max_num_reorder_pics[i]
    w.copy_unsigned_golomb(r); // sps_max_latency_increase[i]
  }

  sps.log2_min_luma_coding_block_size_minus3   = w.copy_unsigned_golomb(r); // log2_min_luma_coding_block_size_minus3
  sps.log2_diff_max_min_luma_coding_block_size = w.copy_unsigned_golomb(r); // log2_diff_max_min_luma_coding_block_size

  w.copy_unsigned_golomb(r); // log2_min_transform_block_size_minus2
  w.copy_unsigned_golomb(r); // log2_diff_max_min_transform_block_size
  w.copy_unsigned_golomb(r); // max_transform_hierarchy_depth_inter
  w.copy_unsigned_golomb(r); // max_transform_hierarchy_depth_intra

  if (w.copy_bits(1, r) == 1)   // scaling_list_enabled_flag
    if (w.copy_bits(1, r) == 1) // sps_scaling_list_data_present_flag
      scaling_list_data_copy(r, w);

  w.copy_bits(1, r);  // amp_enabled_flag
  w.copy_bits(1, r);  // sample_adaptive_offset_enabled_flag

  if (w.copy_bits(1, r) == 1) { // pcm_enabled_flag
    w.copy_bits(4, r);          // pcm_sample_bit_depth_luma_minus1
    w.copy_bits(4, r);          // pcm_sample_bit_depth_chroma_minus1
    w.copy_unsigned_golomb(r);  // log2_min_pcm_luma_coding_block_size_minus3
    w.copy_unsigned_golomb(r);  // log2_diff_max_min_pcm_luma_coding_block_size
    w.copy_bits(1, r);          // pcm_loop_filter_disable_flag
  }

  auto num_short_term_ref_pic_sets = w.copy_unsigned_golomb(r);  // num_short_term_ref_pic_sets

  for (i = 0; i < num_short_term_ref_pic_sets; i++) {
    short_term_ref_pic_set_copy(r, w, sps.short_term_ref_pic_sets, i, num_short_term_ref_pic_sets); // short_term_ref_pic_set(i)
  }

  if (w.copy_bits(1, r) == 1) { // long_term_ref_pics_present_flag
    auto num_long_term_ref_pic_sets = w.copy_unsigned_golomb(r); // num_long_term_ref_pic_sets

    for (i = 0; i < num_long_term_ref_pic_sets; i++) {
      w.copy_bits(sps.log2_max_pic_order_cnt_lsb, r);  // lt_ref_pic_poc_lsb_sps[i]
      w.copy_bits(1, r);                               // used_by_curr_pic_lt_sps_flag[i]
    }
  }

  w.copy_bits(1, r);                   // sps_temporal_mvp_enabled_flag
  w.copy_bits(1, r);                   // strong_intra_smoothing_enabled_flag
  sps.vui_present = w.copy_bits(1, r); // vui_parameters_present_flag

  if (sps.vui_present == 1) {
    vui_parameters_copy(r, w, sps, keep_ar_info, sps.max_sub_layers_minus1);  //vui_parameters()
  }

  if (w.copy_bits(1, r) == 1) // sps_extension_flag
    while (r.get_remaining_bits())
      w.copy_bits(1, r);  // sps_extension_data_flag

  w.put_bit(true);
  w.byte_align();

  // We potentially changed the NALU data with regards to the handling of keep_ar_info.
  // Therefore, we return the changed NALU that exists in w.
  auto new_sps = w.get_buffer();
  sps.checksum = mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, *new_sps);

  // See ITU-T H.265 section 7.4.3.2 for the width/height calculation
  // formula.
  if (sps.conformance_window_flag) {
    auto sub_width_c  = ((1 == sps.chroma_format_idc) || (2 == sps.chroma_format_idc)) && (0 == sps.separate_color_plane_flag) ? 2 : 1;
    auto sub_height_c =  (1 == sps.chroma_format_idc)                                  && (0 == sps.separate_color_plane_flag) ? 2 : 1;

    sps.width        -= std::min<unsigned int>((sub_width_c  * sps.conf_win_right_offset)  + (sub_width_c  * sps.conf_win_left_offset), sps.width);
    sps.height       -= std::min<unsigned int>((sub_height_c * sps.conf_win_bottom_offset) + (sub_height_c * sps.conf_win_top_offset),  sps.height);
  }

  return new_sps;
}

memory_cptr
parse_sps(memory_cptr const &buffer,
          sps_info_t &sps,
          std::vector<vps_info_t> &vps_info_list,
          bool keep_ar_info) {
  try {
    return parse_sps_internal(buffer, sps, vps_info_list, keep_ar_info);
  } catch (mtx::mm_io::exception &) {
  }

  return {};
}

bool
parse_pps(memory_cptr const &buffer,
          pps_info_t &pps) {
  try {
    mtx::bits::reader_c r(buffer->get_buffer(), buffer->get_size());

    pps.clear();

    r.skip_bits(1);                                // forbidden_zero_bit
    if (r.get_bits(6) != NALU_TYPE_PIC_PARAM) // nal_unit_type
      return false;
    r.skip_bits(6);             // nuh_reserved_zero_6bits
    r.skip_bits(3);             // nuh_temporal_id_plus1

    pps.id                                    = r.get_unsigned_golomb(); // pps_pic_parameter_set_id
    pps.sps_id                                = r.get_unsigned_golomb(); // pps_seq_parameter_set_id
    pps.dependent_slice_segments_enabled_flag = r.get_bits(1);           // dependent_slice_segments_enabled_flag
    pps.output_flag_present_flag              = r.get_bits(1);           // output_flag_present_flag
    pps.num_extra_slice_header_bits           = r.get_bits(3);           // num_extra_slice_header_bits

    pps.checksum                              = mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, *buffer);

    return true;
  } catch (...) {
    return false;
  }
}

// HEVC spec, 7.3.2.4
bool
parse_sei(memory_cptr const &buffer,
          user_data_t &user_data) {
  try {
    mtx::bits::reader_c r(buffer->get_buffer(), buffer->get_size());
    mm_mem_io_c byte_reader{*buffer};

    unsigned int bytes_read   = 0;
    unsigned int buffer_size  = buffer->get_size();
    unsigned int payload_type = 0;
    unsigned int payload_size = 0;

    r.skip_bits(1);                                 // forbidden_zero_bit
    if (r.get_bits(6) != NALU_TYPE_PREFIX_SEI) // nal_unit_type
      return false;
    r.skip_bits(6);             // nuh_reserved_zero_6bits
    r.skip_bits(3);             // nuh_temporal_id_plus1

    byte_reader.skip(2);        // skip the nalu header
    bytes_read += 2;

    while(bytes_read < buffer_size-2) {
      payload_type = 0;

      unsigned int payload_type_byte = byte_reader.read_uint8();
      bytes_read++;

      while(payload_type_byte == 0xFF) {
        payload_type     += 255;
        payload_type_byte = byte_reader.read_uint8();
        bytes_read++;
      }

      payload_type += payload_type_byte;
      payload_size  = 0;

      unsigned int payload_size_byte = byte_reader.read_uint8();
      bytes_read++;

      while(payload_size_byte == 0xFF) {
        payload_size     += 255;
        payload_size_byte = byte_reader.read_uint8();
        bytes_read++;
      }
      payload_size += payload_size_byte;

      handle_sei_payload(byte_reader, payload_type, payload_size, user_data);

      bytes_read += payload_size;
    }

    return true;
  } catch (...) {
    return false;
  }
}

bool
handle_sei_payload(mm_mem_io_c &byte_reader,
                   unsigned int sei_payload_type,
                   unsigned int sei_payload_size,
                   user_data_t &user_data) {
  std::vector<unsigned char> uuid;
  uint64_t file_pos = byte_reader.getFilePointer();

  uuid.resize(16);
  if (sei_payload_type == SEI_USER_DATA_UNREGISTERED) {
    if (sei_payload_size >= 16) {
      byte_reader.read(&uuid[0], 16); // uuid

      if (user_data.find(uuid) == user_data.end()) {
        std::vector<unsigned char> &payload = user_data[uuid];

        payload.resize(sei_payload_size);
        memcpy(&payload[0], &uuid[0], 16);
        byte_reader.read(&payload[16], sei_payload_size - 16);
      }
    }
  }

  // Go to end of SEI data by going to its beginning and then skipping over it.
  byte_reader.setFilePointer(file_pos);
  byte_reader.skip(sei_payload_size);

  return true;
}

/** Extract the pixel aspect ratio from the HEVC codec data

   This function searches a buffer containing the HEVC
   codec initialization for the pixel aspectc ratio. If it is found
   then the numerator and the denominator are extracted, and the
   aspect ratio information is removed from the buffer. A structure
   containing the new buffer, the numerator/denominator and the
   success status is returned.

   \param buffer The buffer containing the HEVC codec data.

   \return A \c par_extraction_t structure.
*/
par_extraction_t
extract_par(memory_cptr const &buffer) {
  static debugging_option_c s_debug_ar{"extract_par|hevc_sps|sps_aspect_ratio"};

  try {
    auto hevcc     = hevcc_c::unpack(buffer);
    auto new_hevcc = hevcc;
    bool ar_found = false;
    unsigned int par_num = 1;
    unsigned int par_den = 1;

    new_hevcc.m_sps_list.clear();

    for (auto &nalu : hevcc.m_sps_list) {
      if (!ar_found) {
        try {
          sps_info_t sps_info;
          auto parsed_nalu = parse_sps(mpeg::nalu_to_rbsp(nalu), sps_info, new_hevcc.m_vps_info_list);

          if (parsed_nalu) {
            if (s_debug_ar)
              sps_info.dump();

            ar_found = sps_info.ar_found;
            if (ar_found) {
              par_num = sps_info.par_num;
              par_den = sps_info.par_den;
            }

            nalu = mpeg::rbsp_to_nalu(parsed_nalu);
          }
        } catch (mtx::mm_io::end_of_file_x &) {
        }
      }

      new_hevcc.m_sps_list.push_back(nalu);
    }

    if (!new_hevcc)
      return par_extraction_t{buffer, 0, 0, false};

    return par_extraction_t{new_hevcc.pack(), ar_found ? par_num : 0, ar_found ? par_den : 0, true};

  } catch(...) {
    return par_extraction_t{buffer, 0, 0, false};
  }
}

bool
is_fourcc(const char *fourcc) {
  return balg::to_lower_copy(std::string{fourcc, 4}) == "hevc";
}

memory_cptr
hevcc_to_nalus(const unsigned char *buffer,
               size_t size) {
  try {
    if (6 > size)
      throw false;

    uint32_t marker = get_uint32_be(buffer);
    if (((marker & 0xffffff00) == 0x00000100) || (0x00000001 == marker))
      return memory_c::clone(buffer, size);

    mm_mem_io_c mem(buffer, size);
    mtx::bytes::buffer_c nalus(size * 2);

    if (0x01 != mem.read_uint8())
      throw false;

    mem.setFilePointer(4);
    size_t nal_size_size = 1 + (mem.read_uint8() & 3);
    if (2 > nal_size_size)
      throw false;

    size_t sps_or_pps;
    for (sps_or_pps = 0; 2 > sps_or_pps; ++sps_or_pps) {
      unsigned int num = mem.read_uint8();
      if (0 == sps_or_pps)
        num &= 0x1f;

      size_t i;
      for (i = 0; num > i; ++i) {
        uint16_t element_size   = mem.read_uint16_be();
        memory_cptr copy_buffer = memory_c::alloc(element_size + 4);
        if (element_size != mem.read(copy_buffer->get_buffer() + 4, element_size))
          throw false;

        put_uint32_be(copy_buffer->get_buffer(), mtx::avc_hevc::NALU_START_CODE);
        nalus.add(copy_buffer->get_buffer(), element_size + 4);
      }
    }

    if (mem.getFilePointer() == size)
      return memory_c::clone(nalus.get_buffer(), nalus.get_size());

  } catch (...) {
  }

  return memory_cptr{};
}

bool
parse_dovi_rpu(memory_cptr const &buffer, mtx::dovi::dovi_rpu_data_header_t &hdr) {
  try {
    mtx::bits::reader_c r(buffer->get_buffer(), buffer->get_size());
    mm_mem_io_c byte_reader{*buffer};

    r.skip_bits(1);             // forbidden_zero_bit

    if (r.get_bits(6) != NALU_TYPE_UNSPEC62) // nal_unit_type
      return false;

    r.skip_bits(6);             // nuh_reserved_zero_6bits
    r.skip_bits(3);             // nuh_temporal_id_plus1

    byte_reader.skip(2);        // skip the nalu header

    hdr.rpu_nal_prefix = r.get_bits(8);

    if (hdr.rpu_nal_prefix == 25) {
      hdr.rpu_type   = r.get_bits(6);
      hdr.rpu_format = r.get_bits(11);

      if (hdr.rpu_type == 2) {
        hdr.vdr_rpu_profile           = r.get_bits(4);
        hdr.vdr_rpu_level             = r.get_bits(4);
        hdr.vdr_seq_info_present_flag = r.get_bit();

        if (hdr.vdr_seq_info_present_flag) {
          hdr.chroma_resampling_explicit_filter_flag = r.get_bit();
          hdr.coefficient_data_type                  = r.get_bits(2);

          if (hdr.coefficient_data_type == 0)
            hdr.coefficient_log2_denom = r.get_unsigned_golomb();

          hdr.vdr_rpu_normalized_idc   = r.get_bits(2);
          hdr.bl_video_full_range_flag = r.get_bit();

          if ((hdr.rpu_format & 0x700) == 0) {
            hdr.bl_bit_depth_minus8               = r.get_unsigned_golomb();
            hdr.el_bit_depth_minus8               = r.get_unsigned_golomb();
            hdr.vdr_bit_depth_minus_8             = r.get_unsigned_golomb();
            hdr.spatial_resampling_filter_flag    = r.get_bit();
            hdr.reserved_zero_3bits               = r.get_bits(3);
            hdr.el_spatial_resampling_filter_flag = r.get_bit();
            hdr.disable_residual_flag             = r.get_bit();
          }
        }

        hdr.vdr_dm_metadata_present_flag = r.get_bit();
        hdr.use_prev_vdr_rpu_flag        = r.get_bit();

        if (hdr.use_prev_vdr_rpu_flag)
          hdr.prev_vdr_rpu_id = r.get_unsigned_golomb();

        else {
          hdr.vdr_rpu_id                = r.get_unsigned_golomb();
          hdr.mapping_color_space       = r.get_unsigned_golomb();
          hdr.mapping_chroma_format_idc = r.get_unsigned_golomb();

          for (int cmp = 0; cmp < 3; ++cmp) {
            auto num_pivots_minus2 = r.get_unsigned_golomb();

            for (uint64_t pivot_idx = 0; pivot_idx < num_pivots_minus2 + 2; pivot_idx++)
              r.skip_bits(hdr.bl_bit_depth_minus8 + 8); // pred_pivot_value[cmp][pivot_idx]
          }

          if ((hdr.rpu_format & 0x700) && !hdr.disable_residual_flag)
            r.skip_bits(3); // nlq_method_idc

          hdr.num_x_partitions_minus1 = r.get_unsigned_golomb();
          hdr.num_y_partitions_minus1 = r.get_unsigned_golomb();
        }
      }
    }

    return true;
  } catch (...) {
    return false;
  }
}

}                              // namespace mtx::hevc
