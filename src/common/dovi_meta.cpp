/** DOVI metadata helper functions

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   \file

*/

#include "common/bit_reader.h"
#include "common/bit_writer.h"
#include "common/mm_mem_io.h"
#include "common/codec.h"

#include "common/dovi_meta.h"

namespace mtx::dovi {

void
dovi_rpu_data_header_t::dump() {
  mxinfo(fmt::format("dovi_rpu_data_header\n"
                     "  rpu_nal_prefix: {0}\n"
                     "  rpu_type: {1}\n"
                     "  rpu_format: {2}\n"
                     "  vdr_rpu_profile: {3}\n"
                     "  vdr_rpu_level: {4}\n"
                     "  vdr_seq_info_present_flag: {5}\n"
                     "  chroma_resampling_explicit_filter_flag: {6}\n"
                     "  coefficient_data_type: {7}\n"
                     "  coefficient_log2_denom: {8}\n"
                     "  vdr_rpu_normalized_idc: {9}\n"
                     "  bl_video_full_range_flag: {10}\n"
                     "  bl_bit_depth_minus8: {11}\n"
                     "  el_bit_depth_minus8: {12}\n"
                     "  vdr_bit_depth_minus_8: {13}\n"
                     "  spatial_resampling_filter_flag: {14}\n"
                     "  reserved_zero_3bits: {15}\n"
                     "  el_spatial_resampling_filter_flag: {16}\n"
                     "  disable_residual_flag: {17}\n"
                     "  vdr_dm_metadata_present_flag: {18}\n"
                     "  use_prev_vdr_rpu_flag: {19}\n"
                     "  prev_vdr_rpu_id: {20}\n"
                     "  vdr_rpu_id: {21}\n"
                     "  mapping_color_space: {22}\n"
                     "  mapping_chroma_format_idc: {23}\n"
                     "  num_x_partitions_minus1: {24}\n"
                     "  num_y_partitions_minus1: {25}\n",
                     rpu_nal_prefix,
                     rpu_type,
                     rpu_format,
                     vdr_rpu_profile,
                     vdr_rpu_level,
                     vdr_seq_info_present_flag,
                     chroma_resampling_explicit_filter_flag,
                     coefficient_data_type,
                     coefficient_log2_denom,
                     vdr_rpu_normalized_idc,
                     bl_video_full_range_flag,
                     bl_bit_depth_minus8,
                     el_bit_depth_minus8,
                     vdr_bit_depth_minus_8,
                     spatial_resampling_filter_flag,
                     reserved_zero_3bits,
                     el_spatial_resampling_filter_flag,
                     disable_residual_flag,
                     vdr_dm_metadata_present_flag,
                     use_prev_vdr_rpu_flag,
                     prev_vdr_rpu_id,
                     vdr_rpu_id,
                     mapping_color_space,
                     mapping_chroma_format_idc,
                     num_x_partitions_minus1,
                     num_y_partitions_minus1
                     ));
}

void
dovi_decoder_configuration_record_t::dump() {
  mxinfo(fmt::format("dovi_decoder_configuration_record\n"
                     "  dv_version_major: {0}\n"
                     "  dv_version_minor: {1}\n"
                     "  dv_profile: {2}\n"
                     "  dv_level: {3}\n"
                     "  rpu_present_flag: {4}\n"
                     "  el_present_flag: {5}\n"
                     "  bl_present_flag: {6}\n"
                     "  dv_bl_signal_compatibility_id: {7}\n",
                     dv_version_major,
                     dv_version_minor,
                     dv_profile,
                     dv_level,
                     rpu_present_flag,
                     el_present_flag,
                     bl_present_flag,
                     dv_bl_signal_compatibility_id
                     ));
}

dovi_decoder_configuration_record_t
create_dovi_configuration_record(dovi_rpu_data_header_t const &hdr,
                                 unsigned int width,
                                 unsigned int height,
                                 mtx::hevc::vui_info_t const &vui,
                                 uint64_t duration) {
  dovi_decoder_configuration_record_t conf{};

  bool has_el           = hdr.el_spatial_resampling_filter_flag && !hdr.disable_residual_flag;
  conf.dv_version_major = 1;
  conf.dv_version_minor = 0;

  if (hdr.rpu_nal_prefix == 25) {
    if ((hdr.vdr_rpu_profile == 0) && hdr.bl_video_full_range_flag)
      conf.dv_profile = 5;

    else if (has_el) {
      // Profile 7 is max 12 bits, both MEL & FEL
      if (hdr.vdr_bit_depth_minus_8 == 4)
        conf.dv_profile = 7;
      else
        conf.dv_profile = 4;

    } else
      conf.dv_profile = 8;
  }

  conf.dv_level = calculate_dovi_level(width, height, duration);

  // In all single PID cases, these are set to 1
  conf.rpu_present_flag = 1;
  conf.bl_present_flag  = 1;

  conf.el_present_flag  = 0;

  if (conf.dv_profile == 4)
    conf.dv_bl_signal_compatibility_id = 2;

  else if (conf.dv_profile == 5)
    conf.dv_bl_signal_compatibility_id = 0;

  else if (conf.dv_profile == 8) {
    auto trc = vui.transfer_characteristics;

    // WCG
    if (vui.color_primaries == 9 && vui.matrix_coefficients == 9) {
      if (trc == 16)                       // ST2084, HDR10 base layer
        conf.dv_bl_signal_compatibility_id = 1;
      else if ((trc == 14) || (trc == 18)) // ARIB STD-B67, HLG base layer
        conf.dv_bl_signal_compatibility_id = 4;
      else                                 // undefined
        conf.dv_bl_signal_compatibility_id = 0;

    } else                                 // BT.709, BT.1886, SDR
      conf.dv_bl_signal_compatibility_id = 2;

  } else if (conf.dv_profile == 7)
    conf.dv_bl_signal_compatibility_id = 6;

  else if (conf.dv_profile == 9)
    conf.dv_bl_signal_compatibility_id = 2;

  // Profile 4 is necessarily SDR with an enhancement-layer
  // It's possible that the first guess was wrong, so correct it
  if (has_el && (conf.dv_bl_signal_compatibility_id == 2) && (conf.dv_profile != 4))
    conf.dv_profile = 4;

  // Set EL present for profile 4 and 7
  if ((conf.dv_profile == 4) || (conf.dv_profile == 7)) {
    conf.el_present_flag = 1;
  }

  return conf;
}

uint8_t
calculate_dovi_level(unsigned int width,
                     unsigned int height,
                     uint64_t duration) {
  constexpr auto level1     =    22'118'400ull;
  constexpr auto level2     =    27'648'000ull;
  constexpr auto level3     =    49'766'400ull;
  constexpr auto level4     =    62'208'000ull;
  constexpr auto level5     =   124'416'000ull;
  constexpr auto level6     =   199'065'600ull;
  constexpr auto level7     =   248'832'000ull;
  constexpr auto level8     =   398'131'200ull;
  constexpr auto level9     =   497'664'000ull;
  constexpr auto level10_11 =   995'328'000ull;
  constexpr auto level12    = 1'990'656'000ull;
  constexpr auto level13    = 3'981'312'000ull;

  auto frame_rate           = 1'000'000'000ull / duration;
  auto pps                  = frame_rate * (width * height);

  uint8_t level             = pps <= level1                          ?  1
                            : pps <= level2                          ?  2
                            : pps <= level3                          ?  3
                            : pps <= level4                          ?  4
                            : pps <= level5                          ?  5
                            : pps <= level6                          ?  6
                            : pps <= level7                          ?  7
                            : pps <= level8                          ?  8
                            : pps <= level9                          ?  9
                            : (pps <= level10_11) && (width <= 3840) ? 10
                            : pps <= level10_11                      ? 11
                            : pps <= level12                         ? 12
                            : pps <= level13                         ? 13
                            :                                           0;

  return level;
}

block_addition_mapping_t
create_dovi_block_addition_mapping(dovi_decoder_configuration_record_t const &dovi_conf) {
  block_addition_mapping_t mapping;

  mapping.id_name = "Dolby Vision configuration";
  mapping.id_type = dovi_conf.dv_profile > 7 ? fourcc_c{"dvvC"}.value() : fourcc_c{"dvcC"}.value();

  mtx::bits::writer_c w{};

  w.put_bits(8, dovi_conf.dv_version_major);
  w.put_bits(8, dovi_conf.dv_version_minor);
  w.put_bits(7, dovi_conf.dv_profile);
  w.put_bits(6, dovi_conf.dv_level);
  w.put_bit(dovi_conf.rpu_present_flag);
  w.put_bit(dovi_conf.el_present_flag);
  w.put_bit(dovi_conf.bl_present_flag);
  w.put_bits(4, dovi_conf.dv_bl_signal_compatibility_id);

  w.put_bits(28, 0); // reserved
  w.put_bits(32, 0); // reserved
  w.put_bits(32, 0);
  w.put_bits(32, 0);
  w.put_bits(32, 0);

  w.byte_align();

  mapping.id_extra_data = w.get_buffer();

  return mapping;
}

}                              // namespace mtx::dovi
