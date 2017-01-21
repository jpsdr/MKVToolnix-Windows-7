/** MPEG video helper functions (MPEG 1, 2 and 4)

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   \file

   \author Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <cmath>
#include <unordered_map>

#include "common/bit_cursor.h"
#include "common/byte_buffer.h"
#include "common/checksums/base.h"
#include "common/endian.h"
#include "common/frame_timing.h"
#include "common/hacks.h"
#include "common/mm_io.h"
#include "common/mpeg.h"
#include "common/mpeg4_p10.h"
#include "common/strings/formatting.h"

namespace mpeg4 {
namespace p10 {

static auto s_debug_fix_bistream_timing_info = debugging_option_c{"fix_bitstream_timing_info"};
static auto s_debug_remove_bistream_ar_info  = debugging_option_c{"remove_bitstream_ar_info"};

avcc_c::avcc_c()
  : m_profile_idc{}
  , m_profile_compat{}
  , m_level_idc{}
  , m_nalu_size_length{}
{
}

avcc_c::avcc_c(unsigned int nalu_size_length,
               std::vector<memory_cptr> const &sps_list,
               std::vector<memory_cptr> const &pps_list)
  : m_profile_idc{}
  , m_profile_compat{}
  , m_level_idc{}
  , m_nalu_size_length{nalu_size_length}
  , m_sps_list{sps_list}
  , m_pps_list{pps_list}
{
}

avcc_c::operator bool()
  const {
  return m_nalu_size_length
      && !m_sps_list.empty()
      && !m_pps_list.empty()
      && (m_sps_info_list.empty() || (m_sps_info_list.size() == m_sps_list.size()))
      && (m_pps_info_list.empty() || (m_pps_info_list.size() == m_pps_list.size()));
}

bool
avcc_c::parse_sps_list(bool ignore_errors) {
  if (m_sps_info_list.size() == m_sps_list.size())
    return true;

  m_sps_info_list.clear();
  for (auto &sps: m_sps_list) {
    sps_info_t sps_info;
    auto sps_as_rbsp = mtx::mpeg::nalu_to_rbsp(sps);

    if (ignore_errors) {
      try {
        parse_sps(sps_as_rbsp, sps_info);
      } catch (mtx::mm_io::end_of_file_x &) {
      }
    } else if (!parse_sps(sps_as_rbsp, sps_info))
      return false;

    m_sps_info_list.push_back(sps_info);
  }

  return true;
}

bool
avcc_c::parse_pps_list(bool ignore_errors) {
  if (m_pps_info_list.size() == m_pps_list.size())
    return true;

  m_pps_info_list.clear();
  for (auto &pps: m_pps_list) {
    pps_info_t pps_info;
    auto pps_as_rbsp = mtx::mpeg::nalu_to_rbsp(pps);

    if (ignore_errors) {
      try {
        parse_pps(pps_as_rbsp, pps_info);
      } catch (mtx::mm_io::end_of_file_x &) {
      }
    } else if (!parse_pps(pps_as_rbsp, pps_info))
      return false;

    m_pps_info_list.push_back(pps_info);
  }

  return true;
}

memory_cptr
avcc_c::pack() {
  parse_sps_list(true);
  if (!*this)
    return memory_cptr{};

  unsigned int total_size = 6 + 1;

  for (auto &mem : m_sps_list)
    total_size += mem->get_size() + 2;
  for (auto &mem : m_pps_list)
    total_size += mem->get_size() + 2;

  if (m_trailer)
    total_size += m_trailer->get_size();

  auto destination = memory_c::alloc(total_size);
  auto buffer      = destination->get_buffer();
  auto &sps        = *m_sps_info_list.begin();
  auto write_list  = [&buffer](std::vector<memory_cptr> const &list, uint8_t num_byte_bits) {
    *buffer = list.size() | num_byte_bits;
    ++buffer;

    for (auto &mem : list) {
      auto size = mem->get_size();
      put_uint16_be(buffer, size);
      memcpy(buffer + 2, mem->get_buffer(), size);
      buffer += 2 + size;
    }
  };

  buffer[0] = 1;
  buffer[1] = m_profile_idc    ? m_profile_idc    : sps.profile_idc;
  buffer[2] = m_profile_compat ? m_profile_compat : sps.profile_compat;
  buffer[3] = m_level_idc      ? m_level_idc      : sps.level_idc;
  buffer[4] = 0xfc | (m_nalu_size_length - 1);
  buffer   += 5;

  write_list(m_sps_list, 0xe0);
  write_list(m_pps_list, 0x00);

  if (m_trailer)
    memcpy(buffer, m_trailer->get_buffer(), m_trailer->get_size());

  return destination;
}

avcc_c
avcc_c::unpack(memory_cptr const &mem) {
  avcc_c avcc;

  if (!mem || (6 > mem->get_size()))
    return avcc;

  try {
    mm_mem_io_c in{*mem};

    auto read_list = [&in](std::vector<memory_cptr> &list, unsigned int num_entries_mask) {
      auto num_entries = in.read_uint8() & num_entries_mask;
      while (num_entries) {
        auto size = in.read_uint16_be();
        list.push_back(in.read(size));
        --num_entries;
      }
    };

    in.skip(1);                 // always 1
    avcc.m_profile_idc      = in.read_uint8();
    avcc.m_profile_compat   = in.read_uint8();
    avcc.m_level_idc        = in.read_uint8();
    avcc.m_nalu_size_length = (in.read_uint8() & 0x03) + 1;

    read_list(avcc.m_sps_list, 0x0f);
    read_list(avcc.m_pps_list, 0xff);

    if (in.getFilePointer() < static_cast<uint64_t>(in.get_size()))
      avcc.m_trailer = in.read(in.get_size() - in.getFilePointer());

    return avcc;

  } catch (mtx::mm_io::exception &) {
    return avcc_c{};
  }
}




static const struct {
  int numerator, denominator;
} s_predefined_pars[AVC_NUM_PREDEFINED_PARS] = {
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

};
};

static void
hrdcopy(bit_reader_c &r,
        bit_writer_c &w) {
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
slcopy(bit_reader_c &r,
       bit_writer_c &w,
       int size) {
  int delta, next = 8, j;

  for (j = 0; (j < size) && (0 != next); ++j) {
    delta = w.copy_signed_golomb(r);
    next  = (next + delta + 256) % 256;
  }
}

int64_t
mpeg4::p10::timing_info_t::default_duration()
  const {
  return 1000000000ll * num_units_in_tick / time_scale;
}

void
mpeg4::p10::sps_info_t::dump() {
  mxinfo(boost::format("sps_info dump:\n"
                       "  id:                                    %1%\n"
                       "  profile_idc:                           %2%\n"
                       "  profile_compat:                        %3%\n"
                       "  level_idc:                             %4%\n"
                       "  log2_max_frame_num:                    %5%\n"
                       "  pic_order_cnt_type:                    %6%\n"
                       "  log2_max_pic_order_cnt_lsb:            %7%\n"
                       "  offset_for_non_ref_pic:                %8%\n"
                       "  offset_for_top_to_bottom_field:        %9%\n"
                       "  num_ref_frames_in_pic_order_cnt_cycle: %10%\n"
                       "  delta_pic_order_always_zero_flag:      %11%\n"
                       "  frame_mbs_only:                        %12%\n"
                       "  vui_present:                           %13%\n"
                       "  ar_found:                              %14%\n"
                       "  par_num:                               %15%\n"
                       "  par_den:                               %16%\n"
                       "  timing_info_present:                   %17%\n"
                       "  num_units_in_tick:                     %18%\n"
                       "  time_scale:                            %19%\n"
                       "  fixed_frame_rate:                      %20%\n"
                       "  crop_left:                             %21%\n"
                       "  crop_top:                              %22%\n"
                       "  crop_right:                            %23%\n"
                       "  crop_bottom:                           %24%\n"
                       "  width:                                 %25%\n"
                       "  height:                                %26%\n"
                       "  checksum:                              %|27$08x|\n")
         % id
         % profile_idc
         % profile_compat
         % level_idc
         % log2_max_frame_num
         % pic_order_cnt_type
         % log2_max_pic_order_cnt_lsb
         % offset_for_non_ref_pic
         % offset_for_top_to_bottom_field
         % num_ref_frames_in_pic_order_cnt_cycle
         % delta_pic_order_always_zero_flag
         % frame_mbs_only
         % vui_present
         % ar_found
         % par_num
         % par_den
         % timing_info.is_present
         % timing_info.num_units_in_tick
         % timing_info.time_scale
         % timing_info.fixed_frame_rate
         % crop_left
         % crop_top
         % crop_right
         % crop_bottom
         % width
         % height
         % checksum);
}

bool
mpeg4::p10::sps_info_t::timing_info_valid()
  const {
  return timing_info.is_present
      && (0 != timing_info.num_units_in_tick)
      && (0 != timing_info.time_scale);
}

void
mpeg4::p10::pps_info_t::dump() {
  mxinfo(boost::format("pps_info dump:\n"
                       "id: %1%\n"
                       "sps_id: %2%\n"
                       "pic_order_present: %3%\n"
                       "checksum: %|4$08x|\n")
         % id
         % sps_id
         % pic_order_present
         % checksum);
}

void
mpeg4::p10::slice_info_t::dump()
  const {
  mxinfo(boost::format("slice_info dump:\n"
                       "  nalu_type:                  %1%\n"
                       "  nal_ref_idc:                %2%\n"
                       "  type:                       %3%\n"
                       "  pps_id:                     %4%\n"
                       "  frame_num:                  %5%\n"
                       "  field_pic_flag:             %6%\n"
                       "  bottom_field_flag:          %7%\n"
                       "  idr_pic_id:                 %8%\n"
                       "  pic_order_cnt_lsb:          %9%\n"
                       "  delta_pic_order_cnt_bottom: %10%\n"
                       "  delta_pic_order_cnt:        %11%\n"
                       "  first_mb_in_slice:          %12%\n"
                       "  sps:                        %13%\n"
                       "  pps:                        %14%\n")
         % static_cast<unsigned int>(nalu_type)
         % static_cast<unsigned int>(nal_ref_idc)
         % static_cast<unsigned int>(type)
         % static_cast<unsigned int>(pps_id)
         % frame_num
         % field_pic_flag
         % bottom_field_flag
         % idr_pic_id
         % pic_order_cnt_lsb
         % delta_pic_order_cnt_bottom
         % (delta_pic_order_cnt[0] << 8 | delta_pic_order_cnt[1])
         % first_mb_in_slice
         % sps
         % pps);
}

memory_cptr
mpeg4::p10::parse_sps(memory_cptr const &buffer,
                      sps_info_t &sps,
                      bool keep_ar_info,
                      bool fix_bitstream_frame_rate,
                      int64_t duration) {
  static auto s_high_level_profile_ids = std::unordered_map<unsigned int, bool>{
    {  44, true }, {  83, true }, {  86, true }, { 100, true }, { 110, true }, { 118, true }, { 122, true }, { 128, true }, { 244, true }
  };

  int const add_space   = 100;
  int size              = buffer->get_size();
  auto mcptr_newsps     = memory_c::alloc(size + add_space);
  auto newsps           = mcptr_newsps->get_buffer();
  bit_reader_c r(buffer->get_buffer(), size);
  bit_writer_c w(newsps, size + add_space);
  int i, nref, mb_width, mb_height;

  keep_ar_info = !hack_engaged(ENGAGE_REMOVE_BITSTREAM_AR_INFO);

  memset(&sps, 0, sizeof(sps));

  sps.par_num  = 1;
  sps.par_den  = 1;
  sps.ar_found = false;

  int num_units_in_tick = 0, time_scale = 0;

  if (-1 == duration)
    fix_bitstream_frame_rate = false;

  else {
    auto frame_rate = mtx::frame_timing::determine_frame_rate(duration);
    if (!frame_rate)
      frame_rate.assign(0x80000000, (duration * 0x80000000) / 1000000000ll);

    num_units_in_tick = frame_rate.denominator();
    time_scale        = frame_rate.numerator();
  }

  mxdebug_if(s_debug_fix_bistream_timing_info, boost::format("fix_bitstream_timing_info: duration %1%: units/tick: %2% time_scale: %3%\n") % duration % num_units_in_tick % time_scale);

  w.copy_bits(3, r);            // forbidden_zero_bit, nal_ref_idc
  if (w.copy_bits(5, r) != 7)   // nal_unit_type
    return {};
  sps.profile_idc    = w.copy_bits(8, r); // profile_idc
  sps.profile_compat = w.copy_bits(8, r); // constraints
  sps.level_idc      = w.copy_bits(8, r); // level_idc
  sps.id             = w.copy_unsigned_golomb(r);      // sps id
  if (s_high_level_profile_ids[sps.profile_idc]) {   // high profile
    if ((sps.chroma_format_idc = w.copy_unsigned_golomb(r)) == 3) // chroma_format_idc
      w.copy_bits(1, r);                  // separate_colour_plane_flag
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
  mxdebug_if(s_debug_fix_bistream_timing_info || s_debug_remove_bistream_ar_info, boost::format("VUI present? %1%\n") % sps.vui_present);
  if (sps.vui_present) {
    w.put_bit(1);
    bool ar_info_present = r.get_bit();
    mxdebug_if(s_debug_remove_bistream_ar_info, boost::format("ar_info_present? %1%\n") % ar_info_present);
    if (ar_info_present) {     // ar_info_present
      int ar_type = r.get_bits(8);

      if (keep_ar_info) {
        w.put_bit(1);
        w.put_bits(8, ar_type);
      } else
        w.put_bit(0);

      sps.ar_found = true;

      if (AVC_EXTENDED_SAR == ar_type) {
        sps.par_num = r.get_bits(16);
        sps.par_den = r.get_bits(16);

        if (keep_ar_info) {
          w.put_bits(16, sps.par_num);
          w.put_bits(16, sps.par_den);
        }

      } else if (AVC_NUM_PREDEFINED_PARS >= ar_type) {
        sps.par_num = s_predefined_pars[ar_type].numerator;
        sps.par_den = s_predefined_pars[ar_type].denominator;

      } else
        sps.ar_found = false;

      mxdebug_if(s_debug_remove_bistream_ar_info,
                 boost::format("keep_ar_info %1% ar_type %2% par_num %3% par_den %4%\n")
                 % keep_ar_info % ar_type % sps.par_num % sps.par_den);

    } else
      w.put_bit(0);             // ar_info_present

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
    }

    mxdebug_if(s_debug_fix_bistream_timing_info,
               boost::format("timing info present? %1% #units %2% time_scale %3% fixed rate %4%\n")
               % sps.timing_info.is_present % sps.timing_info.num_units_in_tick % sps.timing_info.time_scale % sps.timing_info.fixed_frame_rate);

    if (fix_bitstream_frame_rate) {                      // write the new timing info
      w.put_bit(1);                                      // timing_info_present
      w.put_bits(32, num_units_in_tick);                 // num_units_in_tick
      w.put_bits(32, time_scale);                        // time_scale
      w.put_bit(1);                                      // fixed_frame_rate

      sps.timing_info.is_present        = true;
      sps.timing_info.num_units_in_tick = num_units_in_tick;
      sps.timing_info.time_scale        = time_scale;
      sps.timing_info.fixed_frame_rate  = true;

    } else if (sps.timing_info.is_present) {             // copy the old timing info
      w.put_bit(1);                                      // timing_info_present
      w.put_bits(32, sps.timing_info.num_units_in_tick); // num_units_in_tick
      w.put_bits(32, sps.timing_info.time_scale);        // time_scale
      w.put_bit(sps.timing_info.fixed_frame_rate);       // fixed_frame_rate

    } else
      w.put_bit(0);                                      // timing_info_present

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
    w.put_bit(1);                        // vui_present
    w.put_bit(0);                        // aspect_ratio_info_present_flag
    w.put_bit(0);                        // overscan_info_present_flag
    w.put_bit(0);                        // video_signal_type_present_flag
    w.put_bit(0);                        // chroma_loc_info_present_flag

                                         // Timing info
    w.put_bit(1);                        // timing_info_present
    w.put_bits(32, num_units_in_tick);   // num_units_in_tick
    w.put_bits(32, time_scale);          // time_scale
    w.put_bit(1);                        // fixed_frame_rate

    sps.timing_info.is_present        = true;
    sps.timing_info.num_units_in_tick = num_units_in_tick;
    sps.timing_info.time_scale        = time_scale;
    sps.timing_info.fixed_frame_rate  = 1;

    w.put_bit(0);                      // nal_hrd_parameters_present_flag
    w.put_bit(0);                      // vcl_hrd_parameters_present_flag
    w.put_bit(0);                      // pic_struct_present_flag
    w.put_bit(0);                      // bitstream_restriction_flag

  } else
    w.put_bit(0);

  w.put_bit(1);
  w.byte_align();

  mcptr_newsps->resize(w.get_bit_position() / 8);

  sps.checksum = mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, *mcptr_newsps);

  return mcptr_newsps;
}

bool
mpeg4::p10::parse_pps(memory_cptr const &buffer,
                      pps_info_t &pps) {
  try {
    bit_reader_c r(buffer->get_buffer(), buffer->get_size());

    memset(&pps, 0, sizeof(pps));

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
mpeg4::p10::par_extraction_t
mpeg4::p10::extract_par(memory_cptr const &buffer) {
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
          auto nalu_as_rbsp = mpeg4::p10::parse_sps(mtx::mpeg::nalu_to_rbsp(nalu), sps_info);

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
mpeg4::p10::fix_sps_fps(memory_cptr const &buffer,
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
    mxdebug_if(s_debug_fix_bistream_timing_info, boost::format("p_mpeg4_p10_fix_sps_fps: num_sps %1%\n") % num_sps);

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

    return memory_cptr{new memory_c{new_avcc.get_and_lock_buffer(), static_cast<size_t>(new_avcc.getFilePointer())}};

  } catch(...) {
    return memory_cptr{};
  }
}

bool
mpeg4::p10::is_avc_fourcc(const char *fourcc) {
  return !strncasecmp(fourcc, "avc",  3)
      || !strncasecmp(fourcc, "h264", 4)
      || !strncasecmp(fourcc, "x264", 4);
}

memory_cptr
mpeg4::p10::avcc_to_nalus(const unsigned char *buffer,
                          size_t size) {
  try {
    if (6 > size)
      throw false;

    uint32_t marker = get_uint32_be(buffer);
    if (((marker & 0xffffff00) == 0x00000100) || (0x00000001 == marker))
      return memory_c::clone(buffer, size);

    mm_mem_io_c mem(buffer, size);
    byte_buffer_c nalus(size * 2);

    if (0x01 != mem.read_uint8())
      throw false;

    mem.setFilePointer(4, seek_beginning);
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

        put_uint32_be(copy_buffer->get_buffer(), NALU_START_CODE);
        nalus.add(copy_buffer->get_buffer(), element_size + 4);
      }
    }

    if (mem.getFilePointer() == size)
      return memory_c::clone(nalus.get_buffer(), nalus.get_size());

  } catch (...) {
  }

  return memory_cptr{};
}

mpeg4::p10::avc_es_parser_c::avc_es_parser_c()
  : m_nalu_size_length(4)
  , m_keep_ar_info(true)
  , m_fix_bitstream_frame_rate(false)
  , m_avcc_ready(false)
  , m_avcc_changed(false)
  , m_stream_default_duration(-1)
  , m_forced_default_duration(-1)
  , m_container_default_duration(-1)
  , m_frame_number(0)
  , m_num_skipped_frames(0)
  , m_first_keyframe_found(false)
  , m_recovery_point_valid(false)
  , m_b_frames_since_keyframe(false)
  , m_par_found(false)
  , m_max_timecode(0)
  , m_previous_frame_start_in_display_order{}
  , m_stream_position(0)
  , m_parsed_position(0)
  , m_have_incomplete_frame(false)
  , m_ignore_nalu_size_length_errors(false)
  , m_discard_actual_frames(false)
  , m_simple_picture_order{}
  , m_first_cleanup{true}
  , m_debug_keyframe_detection{"avc_parser|avc_keyframe_detection"}
  , m_debug_nalu_types{        "avc_parser|avc_nalu_types"}
  , m_debug_timecodes{         "avc_parser|avc_timecodes"}
  , m_debug_sps_info{          "avc_parser|avc_sps|avc_sps_info"}
  , m_debug_trailing_zero_byte_removal{"avc_parser|avc_trailing_zero_byte_removal"}
  , m_debug_sps_pps_changes{           "avc_parser|avc_sps_pps_changes"}
{
  if (m_debug_nalu_types)
    init_nalu_names();
}

mpeg4::p10::avc_es_parser_c::~avc_es_parser_c() {
  mxdebug_if(debugging_c::requested("avc_statistics"),
             boost::format("AVC statistics: #frames: out %1% discarded %2% #timecodes: in %3% generated %4% discarded %5% num_fields: %6% num_frames: %7% num_sei_nalus: %8% num_idr_slices: %9%\n")
             % m_stats.num_frames_out   % m_stats.num_frames_discarded % m_stats.num_timecodes_in % m_stats.num_timecodes_generated % m_stats.num_timecodes_discarded
             % m_stats.num_field_slices % m_stats.num_frame_slices     % m_stats.num_sei_nalus    % m_stats.num_idr_slices);

  mxdebug_if(m_debug_timecodes, boost::format("stream_position %1% parsed_position %2%\n") % m_stream_position % m_parsed_position);

  if (!debugging_c::requested("avc_num_slices_by_type"))
    return;

  static const char *s_type_names[] = {
    "P",  "B",  "I",  "SP",  "SI",
    "P2", "B2", "I2", "SP2", "SI2",
    "unknown"
  };

  int i;
  mxdebug("mpeg4::p10: Number of slices by type:\n");
  for (i = 0; 10 >= i; ++i)
    if (0 != m_stats.num_slices_by_type[i])
      mxdebug(boost::format("  %1%: %2%\n") % s_type_names[i] % m_stats.num_slices_by_type[i]);
}

bool
mpeg4::p10::avc_es_parser_c::headers_parsed()
  const {
  return m_avcc_ready
      && !m_sps_info_list.empty()
      && (m_sps_info_list.front().width  > 0)
      && (m_sps_info_list.front().height > 0);
}

void
mpeg4::p10::avc_es_parser_c::discard_actual_frames(bool discard) {
  m_discard_actual_frames = discard;
}

void
mpeg4::p10::avc_es_parser_c::remove_trailing_zero_bytes(memory_c &memory) {
  auto size = memory.get_size();

  if (size < 3)
    return;

  auto start = memory.get_buffer();
  auto end   = start + size - 3;

  if (get_uint24_be(end) != 0x000000)
    return;

  --end;

  while ((end > start) && (!*end))
    --end;

  auto new_size = end - start + 1;
  memory.set_size(new_size);

  mxdebug_if(m_debug_trailing_zero_byte_removal, boost::format("Removing trailing zero bytes from old size %1% down to new size %2%, removed %3%\n") % size % new_size % (size - new_size));
}

void
mpeg4::p10::avc_es_parser_c::add_bytes(unsigned char *buffer,
                                       size_t size) {
  memory_slice_cursor_c cursor;
  int marker_size              = 0;
  int previous_marker_size     = 0;
  int previous_pos             = -1;
  uint64_t previous_parsed_pos = m_parsed_position;

  if (m_unparsed_buffer && (0 != m_unparsed_buffer->get_size()))
    cursor.add_slice(m_unparsed_buffer);
  cursor.add_slice(buffer, size);

  if (3 <= cursor.get_remaining_size()) {
    uint32_t marker =                               1 << 24
                    | (unsigned int)cursor.get_char() << 16
                    | (unsigned int)cursor.get_char() <<  8
                    | (unsigned int)cursor.get_char();

    while (1) {
      if (NALU_START_CODE == marker)
        marker_size = 4;
      else if (NALU_START_CODE == (marker & 0x00ffffff))
        marker_size = 3;

      if (0 != marker_size) {
        if (-1 != previous_pos) {
          int new_size = cursor.get_position() - marker_size - previous_pos - previous_marker_size;
          auto nalu = memory_c::alloc(new_size);
          cursor.copy(nalu->get_buffer(), previous_pos + previous_marker_size, new_size);
          m_parsed_position = previous_parsed_pos + previous_pos;
          remove_trailing_zero_bytes(*nalu);
          handle_nalu(nalu);
        }
        previous_pos         = cursor.get_position() - marker_size;
        previous_marker_size = marker_size;
        marker_size          = 0;
      }

      if (!cursor.char_available())
        break;

      marker <<= 8;
      marker  |= (unsigned int)cursor.get_char();
    }
  }

  if (-1 == previous_pos)
    previous_pos = 0;

  m_stream_position += size;
  m_parsed_position  = previous_parsed_pos + previous_pos;

  int new_size = cursor.get_size() - previous_pos;
  if (0 != new_size) {
    m_unparsed_buffer = memory_c::alloc(new_size);
    cursor.copy(m_unparsed_buffer->get_buffer(), previous_pos, new_size);

  } else
    m_unparsed_buffer.reset();
}

void
mpeg4::p10::avc_es_parser_c::flush() {
  if (m_unparsed_buffer && (5 <= m_unparsed_buffer->get_size())) {
    m_parsed_position += m_unparsed_buffer->get_size();
    int marker_size = get_uint32_be(m_unparsed_buffer->get_buffer()) == NALU_START_CODE ? 4 : 3;
    handle_nalu(memory_c::clone(m_unparsed_buffer->get_buffer() + marker_size, m_unparsed_buffer->get_size() - marker_size));
  }

  m_unparsed_buffer.reset();
  if (m_have_incomplete_frame) {
    m_frames.push_back(m_incomplete_frame);
    m_have_incomplete_frame = false;
  }

  cleanup();
}

void
mpeg4::p10::avc_es_parser_c::add_timecode(int64_t timecode) {
  m_provided_timecodes.push_back(timecode);
  m_provided_stream_positions.push_back(m_stream_position);
  ++m_stats.num_timecodes_in;
}

bool
mpeg4::p10::avc_es_parser_c::flush_decision(slice_info_t &si,
                                            slice_info_t &ref) {

  if (NALU_TYPE_IDR_SLICE == si.nalu_type) {
    if (0 != si.first_mb_in_slice)
      return false;

    if ((NALU_TYPE_IDR_SLICE == ref.nalu_type) && (si.idr_pic_id != ref.idr_pic_id))
      return true;

    if (NALU_TYPE_IDR_SLICE != ref.nalu_type)
      return true;
  }

  if (si.frame_num != ref.frame_num)
    return true;
  if (si.field_pic_flag != ref.field_pic_flag)
    return true;
  if ((si.nal_ref_idc != ref.nal_ref_idc) && (!si.nal_ref_idc || !ref.nal_ref_idc))
    return true;

  if (m_sps_info_list[si.sps].pic_order_cnt_type == m_sps_info_list[ref.sps].pic_order_cnt_type) {
    if (!m_sps_info_list[ref.sps].pic_order_cnt_type) {
      if (si.pic_order_cnt_lsb != ref.pic_order_cnt_lsb)
        return true;
      if (si.delta_pic_order_cnt_bottom != ref.delta_pic_order_cnt_bottom)
        return true;

    } else if ((1 == m_sps_info_list[ref.sps].pic_order_cnt_type)
               && (   (si.delta_pic_order_cnt[0] != ref.delta_pic_order_cnt[0])
                   || (si.delta_pic_order_cnt[1] != ref.delta_pic_order_cnt[1])))
      return true;
  }

  return false;
}

void
mpeg4::p10::avc_es_parser_c::flush_incomplete_frame() {
  if (!m_have_incomplete_frame || !m_avcc_ready)
    return;

  m_frames.push_back(m_incomplete_frame);
  m_incomplete_frame.clear();
  m_have_incomplete_frame = false;
}

void
mpeg4::p10::avc_es_parser_c::flush_unhandled_nalus() {
  std::deque<memory_cptr>::iterator nalu = m_unhandled_nalus.begin();

  while (m_unhandled_nalus.end() != nalu) {
    handle_nalu(*nalu);
    nalu++;
  }

  m_unhandled_nalus.clear();
}

void
mpeg4::p10::avc_es_parser_c::add_sps_and_pps_to_extra_data() {
  mxdebug_if(m_debug_sps_pps_changes, boost::format("mpeg4::p10: adding all SPS & PPS before key frame due to changes from AVCC\n"));

  brng::remove_erase_if(m_extra_data, [this](memory_cptr const &nalu) -> bool {
    if (nalu->get_size() < static_cast<std::size_t>(m_nalu_size_length + 1))
      return true;

    auto const type = *(nalu->get_buffer() + m_nalu_size_length) & 0x1f;
    return (type == NALU_TYPE_SEQ_PARAM) || (type == NALU_TYPE_PIC_PARAM);
  });

  brng::transform(m_sps_list, std::back_inserter(m_extra_data), [this](memory_cptr const &nalu) { return create_nalu_with_size(nalu); });
  brng::transform(m_pps_list, std::back_inserter(m_extra_data), [this](memory_cptr const &nalu) { return create_nalu_with_size(nalu); });
}

void
mpeg4::p10::avc_es_parser_c::handle_slice_nalu(memory_cptr const &nalu) {
  if (!m_avcc_ready) {
    m_unhandled_nalus.push_back(nalu);
    return;
  }

  slice_info_t si;
  if (!parse_slice(mtx::mpeg::nalu_to_rbsp(nalu), si))
    return;

  if (NALU_TYPE_IDR_SLICE == si.nalu_type)
    ++m_stats.num_idr_slices;

  if (m_have_incomplete_frame && flush_decision(si, m_incomplete_frame.m_si))
    flush_incomplete_frame();

  if (m_have_incomplete_frame) {
    memory_c &mem = *(m_incomplete_frame.m_data.get());
    int offset    = mem.get_size();
    mem.resize(offset + m_nalu_size_length + nalu->get_size());
    mtx::mpeg::write_nalu_size(mem.get_buffer() + offset, nalu->get_size(), m_nalu_size_length, m_ignore_nalu_size_length_errors);
    memcpy(mem.get_buffer() + offset + m_nalu_size_length, nalu->get_buffer(), nalu->get_size());

    return;
  }

  bool is_i_slice =  (AVC_SLICE_TYPE_I   == si.type)
                  || (AVC_SLICE_TYPE2_I  == si.type)
                  || (AVC_SLICE_TYPE_SI  == si.type)
                  || (AVC_SLICE_TYPE2_SI == si.type);
  bool is_b_slice =  (AVC_SLICE_TYPE_B   == si.type)
                  || (AVC_SLICE_TYPE2_B  == si.type);

  m_incomplete_frame.m_si       =  si;
  m_incomplete_frame.m_keyframe =  m_recovery_point_valid
                                || (   is_i_slice
                                    && (NALU_TYPE_IDR_SLICE == si.nalu_type));
  m_incomplete_frame.m_type     =  m_incomplete_frame.m_keyframe ? 'I' : is_b_slice ? 'B' : 'P';
  m_recovery_point_valid        =  false;

  if (m_incomplete_frame.m_keyframe) {
    mxdebug_if(m_debug_keyframe_detection,
               boost::format("AVC:handle_slice_nalu: have incomplete? %1% current is %2% incomplete key bottom field? %3%\n")
               % m_have_incomplete_frame
               % (!si.field_pic_flag                ? "frame" : si.bottom_field_flag              ? "bottom field" : "top field")
               % (!m_current_key_frame_bottom_field ? "none"  : *m_current_key_frame_bottom_field ? "bottom field" : "top field"));

    m_first_keyframe_found    = true;
    m_b_frames_since_keyframe = false;
    if (!si.field_pic_flag || !m_current_key_frame_bottom_field || (*m_current_key_frame_bottom_field == si.bottom_field_flag)) {
      cleanup();

      if (m_sps_or_sps_overwritten)
        add_sps_and_pps_to_extra_data();
    }

    if (!si.field_pic_flag)
      m_current_key_frame_bottom_field = boost::none;

    else if (m_current_key_frame_bottom_field && (*m_current_key_frame_bottom_field != si.bottom_field_flag))
      m_current_key_frame_bottom_field = boost::none;

    else
      m_current_key_frame_bottom_field = si.bottom_field_flag;

  } else if (is_b_slice)
    m_b_frames_since_keyframe = true;

  m_incomplete_frame.m_data = create_nalu_with_size(nalu, true);
  m_have_incomplete_frame   = true;

  if (!m_provided_stream_positions.empty() && (m_parsed_position >= m_provided_stream_positions.front())) {
    m_incomplete_frame.m_has_provided_timecode = true;
    m_provided_stream_positions.pop_front();
  }

  ++m_frame_number;
}

void
mpeg4::p10::avc_es_parser_c::handle_sps_nalu(memory_cptr const &nalu) {
  sps_info_t sps_info;

  auto parsed_nalu = parse_sps(mtx::mpeg::nalu_to_rbsp(nalu), sps_info, m_keep_ar_info, m_fix_bitstream_frame_rate, duration_for(0, true));
  if (!parsed_nalu)
    return;

  parsed_nalu = mtx::mpeg::rbsp_to_nalu(parsed_nalu);

  size_t i;
  for (i = 0; m_sps_info_list.size() > i; ++i)
    if (m_sps_info_list[i].id == sps_info.id)
      break;

  bool use_sps_info = true;
  if (m_sps_info_list.size() == i) {
    m_sps_list.push_back(parsed_nalu);
    m_sps_info_list.push_back(sps_info);
    m_avcc_changed = true;

  } else if (m_sps_info_list[i].checksum != sps_info.checksum) {
    mxdebug_if(m_debug_sps_pps_changes, boost::format("mpeg4::p10: SPS ID %|1$04x| changed; checksum old %|2$04x| new %|3$04x|\n") % sps_info.id % m_sps_info_list[i].checksum % sps_info.checksum);

    m_sps_info_list[i]       = sps_info;
    m_sps_list[i]            = parsed_nalu;
    m_avcc_changed           = true;
    m_sps_or_sps_overwritten = true;

  } else
    use_sps_info = false;

  m_extra_data.push_back(create_nalu_with_size(parsed_nalu));

  if (use_sps_info && m_debug_sps_info)
    sps_info.dump();

  if (!use_sps_info)
    return;

  if (   !has_stream_default_duration()
      && sps_info.timing_info_valid()) {
    m_stream_default_duration = sps_info.timing_info.default_duration();
    mxdebug_if(m_debug_timecodes, boost::format("Stream default duration: %1%\n") % m_stream_default_duration);
  }

  if (   !m_par_found
      && sps_info.ar_found
      && (0 != sps_info.par_den)) {
    m_par_found = true;
    m_par       = int64_rational_c(sps_info.par_num, sps_info.par_den);
  }
}

void
mpeg4::p10::avc_es_parser_c::handle_pps_nalu(memory_cptr const &nalu) {
  pps_info_t pps_info;

  if (!parse_pps(mtx::mpeg::nalu_to_rbsp(nalu), pps_info))
    return;

  size_t i;
  for (i = 0; m_pps_info_list.size() > i; ++i)
    if (m_pps_info_list[i].id == pps_info.id)
      break;

  if (m_pps_info_list.size() == i) {
    m_pps_list.push_back(nalu);
    m_pps_info_list.push_back(pps_info);
    m_avcc_changed = true;

  } else if (m_pps_info_list[i].checksum != pps_info.checksum) {
    mxdebug_if(m_debug_sps_pps_changes, boost::format("mpeg4::p10: PPS ID %|1$04x| changed; checksum old %|2$04x| new %|3$04x|\n") % pps_info.id % m_pps_info_list[i].checksum % pps_info.checksum);

    m_pps_info_list[i]       = pps_info;
    m_pps_list[i]            = nalu;
    m_avcc_changed           = true;
    m_sps_or_sps_overwritten = true;
  }

  m_extra_data.push_back(create_nalu_with_size(nalu));
}

void
mpeg4::p10::avc_es_parser_c::handle_sei_nalu(memory_cptr const &nalu) {
  try {
    ++m_stats.num_sei_nalus;

    auto nalu_as_rbsp = mtx::mpeg::nalu_to_rbsp(nalu);

    bit_reader_c r(nalu_as_rbsp->get_buffer(), nalu_as_rbsp->get_size());

    r.skip_bits(8);

    while (1) {
      int ptype = 0;
      int value;
      while ((value = r.get_bits(8)) == 0xff)
        ptype += value;
      ptype += value;

      int psize = 0;
      while ((value = r.get_bits(8)) == 0xff)
        psize += value;
      psize += value;

      if (6 == ptype) {         // recovery point
        m_recovery_point_valid = true;
        return;
      } else if (0x80 == ptype)
        return;

      r.skip_bits(psize * 8);
    }
  } catch (...) {
  }
}

void
mpeg4::p10::avc_es_parser_c::handle_nalu(memory_cptr const &nalu) {
  if (1 > nalu->get_size())
    return;

  int type = *(nalu->get_buffer()) & 0x1f;

  mxdebug_if(m_debug_nalu_types, boost::format("NALU type 0x%|1$02x| (%2%) size %3%\n") % type % get_nalu_type_name(type) % nalu->get_size());

  switch (type) {
    case NALU_TYPE_SEQ_PARAM:
      flush_incomplete_frame();
      handle_sps_nalu(nalu);
      break;

    case NALU_TYPE_PIC_PARAM:
      flush_incomplete_frame();
      handle_pps_nalu(nalu);
      break;

    case NALU_TYPE_END_OF_SEQ:
    case NALU_TYPE_END_OF_STREAM:
    case NALU_TYPE_ACCESS_UNIT:
      flush_incomplete_frame();
      break;

    case NALU_TYPE_FILLER_DATA:
      // Skip these.
      break;

    case NALU_TYPE_NON_IDR_SLICE:
    case NALU_TYPE_DP_A_SLICE:
    case NALU_TYPE_DP_B_SLICE:
    case NALU_TYPE_DP_C_SLICE:
    case NALU_TYPE_IDR_SLICE:
      if (!m_avcc_ready && !m_sps_info_list.empty() && !m_pps_info_list.empty()) {
        m_avcc_ready = true;
        flush_unhandled_nalus();
      }
      handle_slice_nalu(nalu);
      break;

    default:
      flush_incomplete_frame();
      if (!m_avcc_ready && !m_sps_info_list.empty() && !m_pps_info_list.empty()) {
        m_avcc_ready = true;
        flush_unhandled_nalus();
      }
      m_extra_data.push_back(create_nalu_with_size(nalu));

      if (NALU_TYPE_SEI == type)
        handle_sei_nalu(nalu);

      break;
  }
}

bool
mpeg4::p10::avc_es_parser_c::parse_slice(memory_cptr const &buffer,
                                         slice_info_t &si) {
  try {
    bit_reader_c r(buffer->get_buffer(), buffer->get_size());

    memset(&si, 0, sizeof(si));

    si.nal_ref_idc = r.get_bits(3); // forbidden_zero_bit, nal_ref_idc
    si.nalu_type   = r.get_bits(5); // si.nalu_type
    if (   (NALU_TYPE_NON_IDR_SLICE != si.nalu_type)
        && (NALU_TYPE_DP_A_SLICE    != si.nalu_type)
        && (NALU_TYPE_IDR_SLICE     != si.nalu_type))
      return false;

    si.first_mb_in_slice = r.get_unsigned_golomb(); // first_mb_in_slice
    si.type              = r.get_unsigned_golomb(); // slice_type

    ++m_stats.num_slices_by_type[9 < si.type ? 10 : si.type];

    if (9 < si.type) {
      mxverb(3, boost::format("slice parser error: 9 < si.type: %1%\n") % si.type);
      return false;
    }

    si.pps_id = r.get_unsigned_golomb();      // pps_id

    size_t pps_idx;
    for (pps_idx = 0; m_pps_info_list.size() > pps_idx; ++pps_idx)
      if (m_pps_info_list[pps_idx].id == si.pps_id)
        break;
    if (m_pps_info_list.size() == pps_idx) {
      mxverb(3, boost::format("slice parser error: PPS not found: %1%\n") % si.pps_id);
      return false;
    }

    pps_info_t &pps = m_pps_info_list[pps_idx];
    size_t sps_idx;
    for (sps_idx = 0; m_sps_info_list.size() > sps_idx; ++sps_idx)
      if (m_sps_info_list[sps_idx].id == pps.sps_id)
        break;
    if (m_sps_info_list.size() == sps_idx)
      return false;

    si.sps = sps_idx;
    si.pps = pps_idx;

    sps_info_t &sps = m_sps_info_list[sps_idx];

    si.frame_num = r.get_bits(sps.log2_max_frame_num);

    if (!sps.frame_mbs_only) {
      si.field_pic_flag = r.get_bit();
      if (si.field_pic_flag)
        si.bottom_field_flag = r.get_bit();
    }

    if (NALU_TYPE_IDR_SLICE == si.nalu_type)
      si.idr_pic_id = r.get_unsigned_golomb();

    if (0 == sps.pic_order_cnt_type) {
      si.pic_order_cnt_lsb = r.get_bits(sps.log2_max_pic_order_cnt_lsb);
      if (pps.pic_order_present && !si.field_pic_flag)
        si.delta_pic_order_cnt_bottom = r.get_signed_golomb();
    }

    if ((1 == sps.pic_order_cnt_type) && !sps.delta_pic_order_always_zero_flag) {
      si.delta_pic_order_cnt[0] = r.get_signed_golomb();
      if (pps.pic_order_present && !si.field_pic_flag)
        si.delta_pic_order_cnt[1] = r.get_signed_golomb();
    }

    return true;
  } catch (...) {
    return false;
  }
}

int64_t
mpeg4::p10::avc_es_parser_c::duration_for(unsigned int sps,
                                          bool field_pic_flag)
  const {
  int64_t duration = -1 != m_forced_default_duration                                            ? m_forced_default_duration
                   : (m_sps_info_list.size() > sps) && m_sps_info_list[sps].timing_info_valid() ? m_sps_info_list[sps].timing_info.default_duration()
                   : -1 != m_stream_default_duration                                            ? m_stream_default_duration
                   : -1 != m_container_default_duration                                         ? m_container_default_duration
                   :                                                                              20000000;
  return duration * (field_pic_flag ? 1 : 2);
}

int64_t
mpeg4::p10::avc_es_parser_c::get_most_often_used_duration()
  const {
  int64_t const s_common_default_durations[] = {
    1000000000ll / 50,
    1000000000ll / 25,
    1000000000ll / 60,
    1000000000ll / 30,
    1000000000ll * 1001 / 48000,
    1000000000ll * 1001 / 24000,
    1000000000ll * 1001 / 60000,
    1000000000ll * 1001 / 30000,
  };

  auto most_often = m_duration_frequency.begin();
  for (auto current = m_duration_frequency.begin(); m_duration_frequency.end() != current; ++current)
    if (current->second > most_often->second)
      most_often = current;

  // No duration at all!? No frame?
  if (m_duration_frequency.end() == most_often) {
    mxdebug_if(m_debug_timecodes, boost::format("Duration frequency: none found, using 25 FPS\n"));
    return 1000000000ll / 25;
  }

  auto best = std::make_pair(most_often->first, std::numeric_limits<uint64_t>::max());

  for (auto common_default_duration : s_common_default_durations) {
    uint64_t diff = std::abs(most_often->first - common_default_duration);
    if ((diff < 20000) && (diff < best.second))
      best = std::make_pair(common_default_duration, diff);
  }

  mxdebug_if(m_debug_timecodes, boost::format("Duration frequency. Result: %1%, diff %2%. Best before adjustment: %3%. All: %4%\n")
             % best.first % best.second % most_often->first
             % boost::accumulate(m_duration_frequency, std::string(""), [](std::string const &accu, std::pair<int64_t, int64_t> const &pair) {
                 return accu + (boost::format(" <%1% %2%>") % pair.first % pair.second).str();
               }));

  return best.first;
}

void
mpeg4::p10::avc_es_parser_c::calculate_frame_order() {
  auto frames_begin           = m_frames.begin();
  auto frames_end             = m_frames.end();
  auto frame_itr              = frames_begin;

  auto const &idr             = frame_itr->m_si;
  auto const &sps             = m_sps_info_list[idr.sps];

  auto idx                    = 0u;
  auto prev_pic_order_cnt_msb = 0u;
  auto prev_pic_order_cnt_lsb = 0u;
  auto pic_order_cnt_msb      = 0u;

  while (frames_end != frame_itr) {
    auto const &si = frame_itr->m_si;

    if (si.sps != idr.sps) {
      m_simple_picture_order = true;
      break;
    }

    if (frames_begin == frame_itr)
      ;

    else if ((si.pic_order_cnt_lsb < prev_pic_order_cnt_lsb) && ((prev_pic_order_cnt_lsb - si.pic_order_cnt_lsb) >= (1u << (sps.log2_max_pic_order_cnt_lsb - 1))))
      pic_order_cnt_msb = prev_pic_order_cnt_msb + (1 << sps.log2_max_pic_order_cnt_lsb);

    else if ((si.pic_order_cnt_lsb > prev_pic_order_cnt_lsb) && ((si.pic_order_cnt_lsb - prev_pic_order_cnt_lsb) > (1u << (sps.log2_max_pic_order_cnt_lsb - 1))))
      pic_order_cnt_msb = prev_pic_order_cnt_msb - (1 << sps.log2_max_pic_order_cnt_lsb);

    else
      pic_order_cnt_msb = prev_pic_order_cnt_msb;

    frame_itr->m_presentation_order = pic_order_cnt_msb + si.pic_order_cnt_lsb;
    frame_itr->m_decode_order       = idx;

    ++frame_itr;
    ++idx;

    if (0 != si.nal_ref_idc) {
      prev_pic_order_cnt_lsb = si.pic_order_cnt_lsb;
      prev_pic_order_cnt_msb = pic_order_cnt_msb;
    }
  }
}

void
mpeg4::p10::avc_es_parser_c::cleanup() {
  auto s_debug_force_simple_picture_order = debugging_option_c{"avc_parser_force_simple_picture_order"};

  if (m_frames.empty())
    return;

  if (m_discard_actual_frames) {
    m_stats.num_frames_discarded    += m_frames.size();
    m_stats.num_timecodes_discarded += m_provided_timecodes.size();

    m_frames.clear();
    m_provided_timecodes.clear();
    m_provided_stream_positions.clear();

    return;
  }

  auto const &idr        = m_frames.begin()->m_si;
  auto const &sps        = m_sps_info_list[idr.sps];
  m_simple_picture_order = false;

  if (   (   (AVC_SLICE_TYPE_I   != idr.type)
          && (AVC_SLICE_TYPE_SI  != idr.type)
          && (AVC_SLICE_TYPE2_I  != idr.type)
          && (AVC_SLICE_TYPE2_SI != idr.type))
      || (0 == idr.nal_ref_idc)
      || (0 != sps.pic_order_cnt_type)) {
    m_simple_picture_order = true;
    // return;
  }

  calculate_frame_order();

  if (!m_simple_picture_order && !s_debug_force_simple_picture_order)
    brng::sort(m_frames, [](const avc_frame_t &f1, const avc_frame_t &f2) { return f1.m_presentation_order < f2.m_presentation_order; });

  brng::sort(m_provided_timecodes);

  auto frames_begin          = m_frames.begin();
  auto frames_end            = m_frames.end();
  auto frame_itr             = frames_begin;
  auto previous_frame_itr    = frames_begin;
  auto provided_timecode_itr = m_provided_timecodes.begin();

  while (frames_end != frame_itr) {
    if (frame_itr->m_has_provided_timecode) {
      frame_itr->m_start = *provided_timecode_itr;
      ++provided_timecode_itr;

      if (frames_begin != frame_itr)
        previous_frame_itr->m_end = frame_itr->m_start;

    } else {
      frame_itr->m_start = frames_begin == frame_itr ? m_max_timecode : previous_frame_itr->m_end;
      ++m_stats.num_timecodes_generated;
    }

    frame_itr->m_end = frame_itr->m_start + duration_for(frame_itr->m_si);

    previous_frame_itr = frame_itr;
    ++frame_itr;
  }

  m_max_timecode = m_frames.back().m_end;
  m_provided_timecodes.erase(m_provided_timecodes.begin(), provided_timecode_itr);

  mxdebug_if(m_debug_timecodes, boost::format("CLEANUP frames <pres_ord dec_ord has_prov_tc tc dur>: %1%\n")
             % boost::accumulate(m_frames, std::string(""), [](std::string const &accu, avc_frame_t const &frame) {
                 return accu + (boost::format(" <%1% %2% %3% %4% %5%>") % frame.m_presentation_order % frame.m_decode_order % frame.m_has_provided_timecode % frame.m_start % (frame.m_end - frame.m_start)).str();
               }));


  if (!m_simple_picture_order)
    brng::sort(m_frames, [](const avc_frame_t &f1, const avc_frame_t &f2) { return f1.m_decode_order < f2.m_decode_order; });

  frames_begin       = m_frames.begin();
  frames_end         = m_frames.end();

  // This may be wrong but is needed for mkvmerge to work correctly
  // (cluster_helper etc).
  if (m_first_cleanup) {
    frames_begin->m_keyframe = true;
    m_first_cleanup          = false;
  }

  // mxinfo(boost::format("frame order calculation\n"));

  for (frame_itr = frames_begin; frames_end != frame_itr; ++frame_itr) {
    // mxinfo(boost::format("  type %4% decode order %1% presentation order %2% timestamp %3%\n") % frame_itr->m_decode_order % frame_itr->m_presentation_order % format_timestamp(frame_itr->m_start) % frame_itr->m_type);

    if (!frame_itr->is_i_frame() && (frames_begin != frame_itr))
      frame_itr->m_ref1 = m_previous_frame_start_in_display_order - frame_itr->m_start;

    m_previous_frame_start_in_display_order = frame_itr->m_start;
    m_duration_frequency[frame_itr->m_end - frame_itr->m_start]++;

    if (frame_itr->m_si.field_pic_flag)
      ++m_stats.num_field_slices;
    else
      ++m_stats.num_field_slices;
  }

  m_stats.num_frames_out += m_frames.size();
  m_frames_out.insert(m_frames_out.end(), frames_begin, frames_end);
  m_frames.clear();
}

memory_cptr
mpeg4::p10::avc_es_parser_c::create_nalu_with_size(const memory_cptr &src,
                                                   bool add_extra_data) {
  auto nalu = mtx::mpeg::create_nalu_with_size(src, m_nalu_size_length, add_extra_data ? m_extra_data : std::vector<memory_cptr>{} );

  if (add_extra_data)
    m_extra_data.clear();

  return nalu;
}

memory_cptr
mpeg4::p10::avc_es_parser_c::get_avcc()
  const {
  return avcc_c{static_cast<unsigned int>(m_nalu_size_length), m_sps_list, m_pps_list}.pack();
}

bool
mpeg4::p10::avc_es_parser_c::has_par_been_found()
  const {
  assert(m_avcc_ready);
  return m_par_found;
}

int64_rational_c const &
mpeg4::p10::avc_es_parser_c::get_par()
  const {
  assert(m_avcc_ready && m_par_found);
  return m_par;
}

std::pair<int64_t, int64_t> const
mpeg4::p10::avc_es_parser_c::get_display_dimensions(int width,
                                                    int height)
  const {
  assert(m_avcc_ready && m_par_found);

  if (0 >= width)
    width = get_width();
  if (0 >= height)
    height = get_height();

  return std::make_pair<int64_t, int64_t>(1 <= m_par ? std::llround(width * boost::rational_cast<double>(m_par)) : width,
                                          1 <= m_par ? height                                                    : std::llround(height / boost::rational_cast<double>(m_par)));
}

size_t
mpeg4::p10::avc_es_parser_c::get_num_field_slices()
  const {
  return m_stats.num_field_slices;
}

size_t
mpeg4::p10::avc_es_parser_c::get_num_frame_slices()
  const {
  return m_stats.num_frame_slices;
}

void
mpeg4::p10::avc_es_parser_c::dump_info()
  const {
  mxinfo("Dumping m_frames_out:\n");
  for (auto &frame : m_frames_out) {
    mxinfo(boost::format("size %1% key %2% start %3% end %4% ref1 %5% adler32 0x%|6$08x|\n")
           % frame.m_data->get_size()
           % frame.m_keyframe
           % format_timestamp(frame.m_start)
           % format_timestamp(frame.m_end)
           % format_timestamp(frame.m_ref1)
           % mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, *frame.m_data));
  }
}

std::string
mpeg4::p10::avc_es_parser_c::get_nalu_type_name(int type)
  const {
  auto name = m_nalu_names_by_type.find(type);
  return (m_nalu_names_by_type.end() == name) ? "unknown" : name->second;
}

void
mpeg4::p10::avc_es_parser_c::init_nalu_names() {
  m_nalu_names_by_type[NALU_TYPE_NON_IDR_SLICE] = "non IDR slice";
  m_nalu_names_by_type[NALU_TYPE_DP_A_SLICE]    = "DP A slice";
  m_nalu_names_by_type[NALU_TYPE_DP_B_SLICE]    = "DP B slice";
  m_nalu_names_by_type[NALU_TYPE_DP_C_SLICE]    = "DP C slice";
  m_nalu_names_by_type[NALU_TYPE_IDR_SLICE]     = "IDR slice";
  m_nalu_names_by_type[NALU_TYPE_SEI]           = "SEI";
  m_nalu_names_by_type[NALU_TYPE_SEQ_PARAM]     = "SEQ param";
  m_nalu_names_by_type[NALU_TYPE_PIC_PARAM]     = "PIC param";
  m_nalu_names_by_type[NALU_TYPE_ACCESS_UNIT]   = "access unit";
  m_nalu_names_by_type[NALU_TYPE_END_OF_SEQ]    = "end of sequence";
  m_nalu_names_by_type[NALU_TYPE_END_OF_STREAM] = "end of stream";
  m_nalu_names_by_type[NALU_TYPE_FILLER_DATA]   = "filler";
}
