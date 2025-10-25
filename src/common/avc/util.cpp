/** MPEG-4 p10 video helper functions

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   \file

   \author Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <cmath>
#include <unordered_map>

#include "common/avc/avcc.h"
#include "common/avc/es_parser.h"
#include "common/avc/util.h"
#include "common/bit_reader.h"
#include "common/bit_writer.h"
#include "common/byte_buffer.h"
#include "common/checksums/base.h"
#include "common/endian.h"
#include "common/frame_timing.h"
#include "common/hacks.h"
#include "common/mm_io.h"
#include "common/mm_mem_io.h"
#include "common/mpeg.h"
#include "common/strings/formatting.h"

namespace mtx::avc {

static auto s_debug_fix_bitstream_timing_info = debugging_option_c{"avc_parser|fix_bitstream_timing_info"};
static auto s_debug_remove_bistream_ar_info   = debugging_option_c{"avc_parser|remove_bitstream_ar_info"};

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
hrdcopy(mtx::bits::reader_c &r,
        mtx::bits::writer_c &w) {
  int ncpb = w.copy_unsigned_golomb(r); // cpb_cnt_minus1
  w.copy_bits(4, r);                    // bit_rate_scale
  w.copy_bits(4, r);                    // cpb_size_scale
  for (int i = 0; i <= ncpb; ++i) {
    w.copy_unsigned_golomb(r);          // bit_rate_value_minus1
    w.copy_unsigned_golomb(r);          // cpb_size_value_minus1
    w.copy_bits(1, r);                  // cbr_flag
  }
  w.copy_bits(5, r);                    // initial_cpb_removal_delay_length_minus1
  w.copy_bits(5, r);                    // cpb_removal_delay_length_minus1
  w.copy_bits(5, r);                    // dpb_output_delay_length_minus1
  w.copy_bits(5, r);                    // time_offset_length
}

static void
slcopy(mtx::bits::reader_c &r,
       mtx::bits::writer_c &w,
       int size) {
  int delta, next = 8, j;

  for (j = 0; (j < size) && (0 != next); ++j) {
    delta = w.copy_signed_golomb(r);
    next  = (next + delta + 256) % 256;
  }
}

int64_t
timing_info_t::default_duration()
  const {
  return 1000000000ll * num_units_in_tick / time_scale;
}

bool
timing_info_t::is_valid()
  const {
  return is_present
      && (0 != num_units_in_tick)
      && (0 != time_scale);
}

void
sps_info_t::dump() {
  mxinfo(fmt::format("sps_info dump:\n"
                     "  id:                                    {0}\n"
                     "  profile_idc:                           {1}\n"
                     "  profile_compat:                        {2}\n"
                     "  level_idc:                             {3}\n"
                     "  log2_max_frame_num:                    {4}\n"
                     "  pic_order_cnt_type:                    {5}\n"
                     "  log2_max_pic_order_cnt_lsb:            {6}\n"
                     "  offset_for_non_ref_pic:                {7}\n"
                     "  offset_for_top_to_bottom_field:        {8}\n"
                     "  num_ref_frames_in_pic_order_cnt_cycle: {9}\n"
                     "  delta_pic_order_always_zero_flag:      {10}\n"
                     "  frame_mbs_only:                        {11}\n"
                     "  vui_present:                           {12}\n"
                     "  ar_found:                              {13}\n"
                     "  par_num:                               {14}\n"
                     "  par_den:                               {15}\n"
                     "  timing_info_present:                   {16}\n"
                     "  num_units_in_tick:                     {17}\n"
                     "  time_scale:                            {18}\n"
                     "  fixed_frame_rate:                      {19}\n"
                     "  crop_left:                             {20}\n"
                     "  crop_top:                              {21}\n"
                     "  crop_right:                            {22}\n"
                     "  crop_bottom:                           {23}\n"
                     "  width:                                 {24}\n"
                     "  height:                                {25}\n"
                     "  checksum:                              {26:08x}\n",
                     id,
                     profile_idc,
                     profile_compat,
                     level_idc,
                     log2_max_frame_num,
                     pic_order_cnt_type,
                     log2_max_pic_order_cnt_lsb,
                     offset_for_non_ref_pic,
                     offset_for_top_to_bottom_field,
                     num_ref_frames_in_pic_order_cnt_cycle,
                     delta_pic_order_always_zero_flag,
                     frame_mbs_only,
                     vui_present,
                     ar_found,
                     par_num,
                     par_den,
                     timing_info.is_present,
                     timing_info.num_units_in_tick,
                     timing_info.time_scale,
                     timing_info.fixed_frame_rate,
                     crop_left,
                     crop_top,
                     crop_right,
                     crop_bottom,
                     width,
                     height,
                     checksum));
}

bool
sps_info_t::timing_info_valid()
  const {
  return timing_info.is_valid();
}

void
pps_info_t::dump() {
  mxinfo(fmt::format("pps_info dump:\n"
                     "id: {0}\n"
                     "sps_id: {1}\n"
                     "pic_order_present: {2}\n"
                     "checksum: {3:08x}\n",
                     id,
                     sps_id,
                     pic_order_present,
                     checksum));
}

static memory_cptr
parse_sps_impl(memory_cptr const &buffer,
               sps_info_t &sps,
               bool keep_ar_info,
               bool fix_bitstream_frame_rate,
               int64_t duration) {
  static auto s_high_level_profile_ids = std::unordered_map<unsigned int, bool>{
    {  44, true }, {  83, true }, {  86, true }, { 100, true }, { 110, true }, { 118, true }, { 122, true }, { 128, true }, { 244, true }
  };

  int size = buffer->get_size();
  mtx::bits::reader_c r(buffer->get_buffer(), size);
  mtx::bits::writer_c w{};
  int i, nref, mb_width, mb_height;

  keep_ar_info = !mtx::hacks::is_engaged(mtx::hacks::REMOVE_BITSTREAM_AR_INFO);

  sps          = sps_info_t{};
  sps.par_num  = 1;
  sps.par_den  = 1;
  sps.ar_found = false;

  int num_units_in_tick = 0, time_scale = 0;

  if (-1 == duration)
    fix_bitstream_frame_rate = false;

  else {
    auto frame_rate = mtx::frame_timing::determine_frame_rate(duration);
    if (!frame_rate) {
      frame_rate = mtx::rational(1'000'000'000ll, duration);
      if (   (boost::multiprecision::denominator(frame_rate) > std::numeric_limits<uint32_t>::max())
          || (boost::multiprecision::numerator(frame_rate)   > std::numeric_limits<uint32_t>::max()))
        frame_rate = mtx::rational(0x80000000, static_cast<int64_t>((duration * 0x80000000) / 1000000000ll));
    }

    num_units_in_tick = static_cast<int>(boost::multiprecision::denominator(frame_rate));
    time_scale        = static_cast<int>(boost::multiprecision::numerator(frame_rate));
  }

  mxdebug_if(s_debug_fix_bitstream_timing_info, fmt::format("fix_bitstream_timing_info: duration {0}: units/tick: {1} time_scale: {2}\n", duration, num_units_in_tick, time_scale));

  w.copy_bits(3, r);            // forbidden_zero_bit, nal_ref_idc
  if (w.copy_bits(5, r) != 7)   // nal_unit_type
    return {};
  sps.profile_idc    = w.copy_bits(8, r); // profile_idc
  sps.profile_compat = w.copy_bits(8, r); // constraints
  sps.level_idc      = w.copy_bits(8, r); // level_idc
  sps.id             = w.copy_unsigned_golomb(r);      // sps id
  if (s_high_level_profile_ids[sps.profile_idc]) {   // high profile
    if ((sps.chroma_format_idc = w.copy_unsigned_golomb(r)) == 3) // chroma_format_idc
      w.copy_bits(1, r);                  // separate_color_plane_flag
    w.copy_unsigned_golomb(r);                         // bit_depth_luma_minus8
    w.copy_unsigned_golomb(r);                         // bit_depth_chroma_minus8
    w.copy_bits(1, r);                    // qpprime_y_zero_transform_bypass_flag
    if (w.copy_bits(1, r) == 1)           // seq_scaling_matrix_present_flag
      for (i = 0; i < 8; ++i)
        if (w.copy_bits(1, r) == 1)       // seq_scaling_list_present_flag
          slcopy(r, w, i < 6 ? 16 : 64);
  } else
    sps.chroma_format_idc = 1;            // 4:2:0 assumed

  sps.log2_max_frame_num = w.copy_unsigned_golomb(r) + 4; // log2_max_frame_num_minus4
  sps.pic_order_cnt_type = w.copy_unsigned_golomb(r);     // pic_order_cnt_type
  switch (sps.pic_order_cnt_type) {
    case 0:
      sps.log2_max_pic_order_cnt_lsb = w.copy_unsigned_golomb(r) + 4; // log2_max_pic_order_cnt_lsb_minus4
      break;

    case 1:
      sps.delta_pic_order_always_zero_flag      = w.copy_bits(1, r); // delta_pic_order_always_zero_flag
      sps.offset_for_non_ref_pic                = w.copy_signed_golomb(r);     // offset_for_non_ref_pic
      sps.offset_for_top_to_bottom_field        = w.copy_signed_golomb(r);     // offset_for_top_to_bottom_field
      nref                                      = w.copy_unsigned_golomb(r);      // num_ref_frames_in_pic_order_cnt_cycle
      sps.num_ref_frames_in_pic_order_cnt_cycle = nref;
      for (i = 0; i < nref; ++i)
        w.copy_unsigned_golomb(r);           // offset_for_ref_frame
      break;

    case 2:
      break;

    default:
      return {};
  }

  w.copy_unsigned_golomb(r);                 // num_ref_frames
  w.copy_bits(1, r);            // gaps_in_frame_num_value_allowed_flag
  mb_width  = w.copy_unsigned_golomb(r) + 1; // pic_width_in_mbs_minus1
  mb_height = w.copy_unsigned_golomb(r) + 1; // pic_height_in_map_units_minus1
  if (w.copy_bits(1, r) == 0) { // frame_mbs_only_flag
    sps.frame_mbs_only = false;
    w.copy_bits(1, r);          // mb_adaptive_frame_field_flag
  } else
    sps.frame_mbs_only = true;

  sps.width  = mb_width * 16;
  sps.height = (2 - sps.frame_mbs_only) * mb_height * 16;

  w.copy_bits(1, r);            // direct_8x8_inference_flag
  if (w.copy_bits(1, r) == 1) { // frame_cropping_flag
    unsigned int crop_unit_x;
    unsigned int crop_unit_y;
    if (0 == sps.chroma_format_idc) { // monochrome
      crop_unit_x = 1;
      crop_unit_y = 2 - sps.frame_mbs_only;
    } else if (1 == sps.chroma_format_idc) { // 4:2:0
      crop_unit_x = 2;
      crop_unit_y = 2 * (2 - sps.frame_mbs_only);
    } else if (2 == sps.chroma_format_idc) { // 4:2:2
      crop_unit_x = 2;
      crop_unit_y = 2 - sps.frame_mbs_only;
    } else { // 3 == sps.chroma_format_idc   // 4:4:4
      crop_unit_x = 1;
      crop_unit_y = 2 - sps.frame_mbs_only;
    }
    sps.crop_left   = w.copy_unsigned_golomb(r); // frame_crop_left_offset
    sps.crop_right  = w.copy_unsigned_golomb(r); // frame_crop_right_offset
    sps.crop_top    = w.copy_unsigned_golomb(r); // frame_crop_top_offset
    sps.crop_bottom = w.copy_unsigned_golomb(r); // frame_crop_bottom_offset

    sps.width      -= crop_unit_x * (sps.crop_left + sps.crop_right);
    sps.height     -= crop_unit_y * (sps.crop_top  + sps.crop_bottom);
  }

  sps.vui_present = r.get_bit();
  mxdebug_if(s_debug_fix_bitstream_timing_info || s_debug_remove_bistream_ar_info, fmt::format("VUI present? {0}\n", sps.vui_present));
  if (sps.vui_present) {
    w.put_bit(true);
    bool ar_info_present = r.get_bit();
    mxdebug_if(s_debug_remove_bistream_ar_info, fmt::format("ar_info_present? {0}\n", ar_info_present));
    if (ar_info_present) {     // ar_info_present
      int ar_type = r.get_bits(8);

      if (keep_ar_info) {
        w.put_bit(true);
        w.put_bits(8, ar_type);
      } else
        w.put_bit(false);

      sps.ar_found = true;

      if (EXTENDED_SAR == ar_type) {
        sps.par_num = r.get_bits(16);
        sps.par_den = r.get_bits(16);

        if (keep_ar_info) {
          w.put_bits(16, sps.par_num);
          w.put_bits(16, sps.par_den);
        }

      } else if (NUM_PREDEFINED_PARS >= ar_type) {
        sps.par_num = s_predefined_pars[ar_type].numerator;
        sps.par_den = s_predefined_pars[ar_type].denominator;

      } else
        sps.ar_found = false;

      mxdebug_if(s_debug_remove_bistream_ar_info,
                 fmt::format("keep_ar_info {0} ar_type {1} par_num {2} par_den {3}\n",
                             keep_ar_info, ar_type, sps.par_num, sps.par_den));

    } else
      w.put_bit(false);         // ar_info_present

    // copy the rest
    if (w.copy_bits(1, r) == 1)   // overscan_info_present
      w.copy_bits(1, r);          // overscan_appropriate
    if (w.copy_bits(1, r) == 1) { // video_signal_type_present
      w.copy_bits(4, r);          // video_format, video_full_range
      if (w.copy_bits(1, r) == 1) // color_desc_present
        w.copy_bits(24, r);
    }
    if (w.copy_bits(1, r) == 1) { // chroma_loc_info_present
      w.copy_unsigned_golomb(r);               // chroma_sample_loc_type_top_field
      w.copy_unsigned_golomb(r);               // chroma_sample_loc_type_bottom_field
    }

    sps.timing_info.is_present = r.get_bit();

    if (sps.timing_info.is_present) {                    // Read original values.
      sps.timing_info.num_units_in_tick = r.get_bits(32);
      sps.timing_info.time_scale        = r.get_bits(32);
      sps.timing_info.fixed_frame_rate  = r.get_bit();

      if (   sps.timing_info.is_valid()
          && (sps.timing_info.default_duration() < 5'000ll)) {
        mxdebug_if(s_debug_fix_bitstream_timing_info,
                   fmt::format("timing info present && bogus values detected (#units {1} time_scale {2}); defaulting to 25 FPS (default duration 20'000'000)\n",
                               sps.timing_info.is_present, sps.timing_info.num_units_in_tick, sps.timing_info.time_scale));
        sps.timing_info.num_units_in_tick = 1;
        sps.timing_info.time_scale        = 50;
      }
    }

    mxdebug_if(s_debug_fix_bitstream_timing_info,
               fmt::format("timing info present? {0} #units {1} time_scale {2} fixed rate {3}\n",
                           sps.timing_info.is_present, sps.timing_info.num_units_in_tick, sps.timing_info.time_scale, sps.timing_info.fixed_frame_rate));

    if (fix_bitstream_frame_rate) {                      // write the new timing info
      w.put_bit(true);                                   // timing_info_present
      w.put_bits(32, num_units_in_tick);                 // num_units_in_tick
      w.put_bits(32, time_scale);                        // time_scale
      w.put_bit(true);                                   // fixed_frame_rate

      sps.timing_info.is_present        = true;
      sps.timing_info.num_units_in_tick = num_units_in_tick;
      sps.timing_info.time_scale        = time_scale;
      sps.timing_info.fixed_frame_rate  = true;

    } else if (sps.timing_info.is_present) {             // copy the old timing info
      w.put_bit(true);                                   // timing_info_present
      w.put_bits(32, sps.timing_info.num_units_in_tick); // num_units_in_tick
      w.put_bits(32, sps.timing_info.time_scale);        // time_scale
      w.put_bit(sps.timing_info.fixed_frame_rate);       // fixed_frame_rate

    } else
      w.put_bit(false);                                  // timing_info_present

    bool f = false;
    if (w.copy_bits(1, r) == 1) { // nal_hrd_parameters_present
      hrdcopy(r, w);
      f = true;
    }
    if (w.copy_bits(1, r) == 1) { // vcl_hrd_parameters_present
      hrdcopy(r, w);
      f = true;
    }
    if (f)
      w.copy_bits(1, r);        // low_delay_hrd_flag
    w.copy_bits(1, r);          // pic_struct_present

    if (w.copy_bits(1, r) == 1) { // bitstream_restriction
      w.copy_bits(1, r);          // motion_vectors_over_pic_boundaries_flag
      w.copy_unsigned_golomb(r);  // max_bytes_per_pic_denom
      w.copy_unsigned_golomb(r);  // max_bits_per_pic_denom
      w.copy_unsigned_golomb(r);  // log2_max_mv_length_h
      w.copy_unsigned_golomb(r);  // log2_max_mv_length_v
      w.copy_unsigned_golomb(r);  // num_reorder_frames
      w.copy_unsigned_golomb(r);  // max_dec_frame_buffering
    }

  } else if (fix_bitstream_frame_rate) { // vui_present == 0: build new video usability information
    w.put_bit(true);                     // vui_present
    w.put_bit(false);                    // aspect_ratio_info_present_flag
    w.put_bit(false);                    // overscan_info_present_flag
    w.put_bit(false);                    // video_signal_type_present_flag
    w.put_bit(false);                    // chroma_loc_info_present_flag

                                         // Timing info
    w.put_bit(true);                     // timing_info_present
    w.put_bits(32, num_units_in_tick);   // num_units_in_tick
    w.put_bits(32, time_scale);          // time_scale
    w.put_bit(true);                     // fixed_frame_rate

    sps.timing_info.is_present        = true;
    sps.timing_info.num_units_in_tick = num_units_in_tick;
    sps.timing_info.time_scale        = time_scale;
    sps.timing_info.fixed_frame_rate  = true;

    w.put_bit(false);                  // nal_hrd_parameters_present_flag
    w.put_bit(false);                  // vcl_hrd_parameters_present_flag
    w.put_bit(false);                  // pic_struct_present_flag
    w.put_bit(false);                  // bitstream_restriction_flag

  } else
    w.put_bit(false);

  w.put_bit(true);
  w.byte_align();

  auto new_sps = w.get_buffer();
  sps.checksum = mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, *new_sps);

  return new_sps;
}

memory_cptr
parse_sps(memory_cptr const &buffer,
          sps_info_t &sps,
          bool keep_ar_info,
          bool fix_bitstream_frame_rate,
          int64_t duration) {
  try {
    return parse_sps_impl(buffer, sps, keep_ar_info, fix_bitstream_frame_rate, duration);

  } catch (mtx::exception &) {
    return {};
  }
}

bool
parse_pps(memory_cptr const &buffer,
          pps_info_t &pps) {
  try {
    mtx::bits::reader_c r(buffer->get_buffer(), buffer->get_size());

    pps = pps_info_t{};

    r.skip_bits(3);             // forbidden_zero_bit, nal_ref_idc
    if (r.get_bits(5) != 8)     // nal_unit_type
      return false;
    pps.id     = r.get_unsigned_golomb();
    pps.sps_id = r.get_unsigned_golomb();

    r.skip_bits(1);             // entropy_coding_mode_flag
    pps.pic_order_present = r.get_bit();
    pps.checksum          = mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, *buffer);

    return true;
  } catch (...) {
    return false;
  }
}

/** Extract the pixel aspect ratio from the MPEG4 layer 10 (AVC) codec data

   This function searches a buffer containing the MPEG4 layer 10 (AVC)
   codec initialization for the pixel aspectc ratio. If it is found
   then the numerator and the denominator are extracted, and the
   aspect ratio information is removed from the buffer. A structure
   containing the new buffer, the numerator/denominator and the
   success status is returned.

   \param buffer The buffer containing the MPEG4 layer 10 codec data.

   \return A \c par_extraction_t structure.
*/
par_extraction_t
extract_par(memory_cptr const &buffer) {
  static debugging_option_c s_debug_ar{"extract_par|avc_sps|sps_aspect_ratio"};

  try {
    auto avcc            = avcc_c::unpack(buffer);
    auto new_avcc        = avcc;
    bool ar_found        = false;
    unsigned int par_num = 1;
    unsigned int par_den = 1;

    new_avcc.m_sps_list.clear();

    for (auto &nalu : avcc.m_sps_list) {
      if (!ar_found) {
        try {
          sps_info_t sps_info;
          auto nalu_as_rbsp = parse_sps(mtx::mpeg::nalu_to_rbsp(nalu), sps_info);

          if (nalu_as_rbsp) {
            if (s_debug_ar)
              sps_info.dump();

            ar_found = sps_info.ar_found;
            if (ar_found) {
              par_num = sps_info.par_num;
              par_den = sps_info.par_den;
            }

            nalu = mtx::mpeg::rbsp_to_nalu(nalu_as_rbsp);
          }
        } catch (mtx::mm_io::end_of_file_x &) {
        }
      }

      new_avcc.m_sps_list.push_back(nalu);
    }

    if (!new_avcc)
      return par_extraction_t{buffer, 0, 0, false};

    return par_extraction_t{new_avcc.pack(), ar_found ? par_num : 0, ar_found ? par_den : 0, true};

  } catch(...) {
    return par_extraction_t{buffer, 0, 0, false};
  }
}

memory_cptr
fix_sps_fps(memory_cptr const &buffer,
            int64_t duration) {
  try {
    auto buffer_size = buffer->get_size();
    mm_mem_io_c avcc(buffer->get_buffer(), buffer->get_size()), new_avcc(nullptr, buffer_size, 1024);
    memory_cptr nalu(new memory_c());

    avcc.read(nalu, 5);
    new_avcc.write(nalu);

    // parse and fix the sps
    unsigned int num_sps = avcc.read_uint8();
    new_avcc.write_uint8(num_sps);
    num_sps &= 0x1f;
    mxdebug_if(s_debug_fix_bitstream_timing_info, fmt::format("p_mpeg4_p10_fix_sps_fps: num_sps {0}\n", num_sps));

    unsigned int sps;
    for (sps = 0; sps < num_sps; sps++) {
      unsigned int length = avcc.read_uint16_be();
      if ((length + avcc.getFilePointer()) >= buffer_size)
        length = buffer_size - avcc.getFilePointer();
      avcc.read(nalu, length);

      if ((0 < length) && ((nalu->get_buffer()[0] & 0x1f) == NALU_TYPE_SEQ_PARAM)) {
        sps_info_t sps_info;
        auto parsed_nalu = parse_sps(mtx::mpeg::nalu_to_rbsp(nalu), sps_info, true, true, duration);

        if (parsed_nalu)
          nalu = mtx::mpeg::rbsp_to_nalu(parsed_nalu);
      }

      new_avcc.write_uint16_be(nalu->get_size());
      new_avcc.write(nalu);
    }

    // copy the pps
    auto remaining_bytes = avcc.get_size() - avcc.getFilePointer();
    if (remaining_bytes) {
      avcc.read(nalu, remaining_bytes);
      new_avcc.write(nalu);
    }

    return new_avcc.get_and_lock_buffer();

  } catch(...) {
    return memory_cptr{};
  }
}

bool
is_avc_fourcc(const char *fourcc) {
  return !strncasecmp(fourcc, "avc",  3)
      || !strncasecmp(fourcc, "h264", 4)
      || !strncasecmp(fourcc, "x264", 4);
}

memory_cptr
avcc_to_nalus(const uint8_t *buffer,
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

        put_uint32_be(copy_buffer->get_buffer(), mtx::xyzvc::NALU_START_CODE);
        nalus.add(copy_buffer->get_buffer(), element_size + 4);
      }
    }

    if (mem.getFilePointer() == size)
      return memory_c::clone(nalus.get_buffer(), nalus.get_size());

  } catch (...) {
  }

  return memory_cptr{};
}

}
