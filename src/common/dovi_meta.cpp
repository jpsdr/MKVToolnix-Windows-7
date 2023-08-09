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

bool
parse_dovi_rpu(mtx::bits::reader_c &r, mtx::dovi::dovi_rpu_data_header_t &hdr) {
  hdr.rpu_nal_prefix = r.get_bits(8);

  if (hdr.rpu_nal_prefix != 25)
    return true;

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

  return true;
}

uint8_t
guess_dovi_rpu_data_header_profile(dovi_rpu_data_header_t const &hdr) {
  bool has_el = hdr.el_spatial_resampling_filter_flag && !hdr.disable_residual_flag;

  if (hdr.rpu_nal_prefix != 25)
    return 0;

  if ((hdr.vdr_rpu_profile == 0) && hdr.bl_video_full_range_flag)
    return 5;

  if (has_el) {
    // Profile 7 is max 12 bits, both MEL & FEL
    if (hdr.vdr_bit_depth_minus_8 == 4)
      return 7;
    else
      return 4;

  } else
    return 8;
}

uint8_t
get_dovi_bl_signal_compatibility_id(uint8_t dv_profile,
                                   unsigned int color_primaries,
                                   unsigned int matrix_coefficients,
                                   unsigned int transfer_characteristics) {
  if (dv_profile == 4)
    return 2;

  if (dv_profile == 5)
    return 0;

  if (dv_profile == 8) {
    auto trc = transfer_characteristics;

    // WCG
    if (color_primaries == 9 && matrix_coefficients == 9) {
      if (trc == 16)                       // ST2084, HDR10 base layer
        return 1;
      else if ((trc == 14) || (trc == 18)) // ARIB STD-B67, HLG base layer
        return 4;
      else                                 // undefined
        return 0;

    } else                                 // BT.709, BT.1886, SDR
      return 2;

  }

  if (dv_profile == 7)
    return 6;

  if (dv_profile == 9)
    return 2;

  return 0;
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

dovi_decoder_configuration_record_t
create_dovi_configuration_record(dovi_rpu_data_header_t const &hdr,
                                 unsigned int width,
                                 unsigned int height,
                                 mtx::hevc::vui_info_t const &vui,
                                 uint64_t duration) {
  dovi_decoder_configuration_record_t conf{};

  bool has_el                        = hdr.el_spatial_resampling_filter_flag && !hdr.disable_residual_flag;
  conf.dv_version_major              = 1;
  conf.dv_version_minor              = 0;

  conf.dv_profile                    = guess_dovi_rpu_data_header_profile(hdr);
  conf.dv_level                      = calculate_dovi_level(width, height, duration);
  conf.dv_bl_signal_compatibility_id = get_dovi_bl_signal_compatibility_id(conf.dv_profile, vui.color_primaries, vui.matrix_coefficients, vui.transfer_characteristics);

  // In all single PID cases, these are set to 1
  conf.rpu_present_flag              = 1;
  conf.bl_present_flag               = 1;
  conf.el_present_flag               = 0;

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

dovi_decoder_configuration_record_t
create_av1_dovi_configuration_record(dovi_rpu_data_header_t const &hdr,
                                     unsigned int width,
                                     unsigned int height,
                                     mtx::av1::color_config_t const &color_config,
                                     uint64_t duration) {
  dovi_decoder_configuration_record_t conf{};

  conf.dv_version_major = 1;
  conf.dv_version_minor = 0;

  conf.dv_profile       = 10; // AV1 profile is always 10
  conf.dv_level         = calculate_dovi_level(width, height, duration);

  conf.rpu_present_flag = 1;
  conf.bl_present_flag  = 1;
  conf.el_present_flag  = 0; // Not sure it's possible to have an EL in AV1

  // Actual profile based on header
  auto dv_profile = guess_dovi_rpu_data_header_profile(hdr);

  conf.dv_bl_signal_compatibility_id = get_dovi_bl_signal_compatibility_id(dv_profile, color_config.color_primaries, color_config.matrix_coefficients, color_config.transfer_characteristics);

  return conf;
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
