/** DOVI metadata helper functions

    mkvmerge -- utility for splicing together matroska files
    from component media subtypes

    Distributed under the GPL v2
    see the file COPYING for details
    or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

    \file

*/

#pragma once

#include "common/bit_reader.h"
#include "common/common_pch.h"
#include "merge/block_addition_mapping.h"
#include "common/av1/types.h"
#include "common/hevc/types.h"

namespace mtx::dovi {

struct dovi_decoder_configuration_record_t {
  uint8_t dv_version_major{};
  uint8_t dv_version_minor{};
  uint8_t dv_profile{};
  uint8_t dv_level{};
  uint8_t rpu_present_flag{};
  uint8_t el_present_flag{};
  uint8_t bl_present_flag{};
  uint8_t dv_bl_signal_compatibility_id{};

  void dump();
};

struct dovi_rpu_data_header_t {
  unsigned int rpu_nal_prefix{};
  unsigned int rpu_type{};
  unsigned int rpu_format{};

  unsigned int vdr_rpu_profile{};
  unsigned int vdr_rpu_level{};
  bool vdr_seq_info_present_flag{};

  bool chroma_resampling_explicit_filter_flag{};
  unsigned int coefficient_data_type{};
  uint64_t coefficient_log2_denom{};
  unsigned int vdr_rpu_normalized_idc{};

  bool bl_video_full_range_flag{};
  uint64_t bl_bit_depth_minus8{};
  uint64_t el_bit_depth_minus8{};
  uint64_t vdr_bit_depth_minus_8{};

  bool spatial_resampling_filter_flag{};
  bool reserved_zero_3bits{};
  bool el_spatial_resampling_filter_flag{};
  bool disable_residual_flag{};

  bool vdr_dm_metadata_present_flag{};
  bool use_prev_vdr_rpu_flag{};
  uint64_t prev_vdr_rpu_id{};
  uint64_t vdr_rpu_id{};

  uint64_t mapping_color_space{};
  uint64_t mapping_chroma_format_idc{};

  uint64_t num_x_partitions_minus1{};
  uint64_t num_y_partitions_minus1{};

  void dump();
};

bool parse_dovi_rpu(mtx::bits::reader_c &r, dovi_rpu_data_header_t &hdr);

uint8_t guess_dovi_rpu_data_header_profile(dovi_rpu_data_header_t const &hdr);
uint8_t get_dovi_bl_signal_compatibility_id(uint8_t dv_profile,
                                            unsigned int color_primaries,
                                            unsigned int matrix_coefficients,
                                            unsigned int transfer_characteristics);
uint8_t calculate_dovi_level(unsigned int width, unsigned int height, uint64_t duration);

dovi_decoder_configuration_record_t create_dovi_configuration_record(dovi_rpu_data_header_t const &hdr, unsigned int width, unsigned int height, mtx::hevc::vui_info_t const &vui, uint64_t duration);
dovi_decoder_configuration_record_t create_av1_dovi_configuration_record(dovi_rpu_data_header_t const &hdr, unsigned int width, unsigned int height, mtx::av1::color_config_t const &color_config, uint64_t duration);
dovi_decoder_configuration_record_t parse_dovi_decoder_configuration_record(mtx::bits::reader_c &r);

block_addition_mapping_t create_dovi_block_addition_mapping(dovi_decoder_configuration_record_t const &dovi_conf);

}                             // namespace mtx::dovi
