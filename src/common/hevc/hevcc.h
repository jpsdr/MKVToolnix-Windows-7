/** MPEG-h HEVCC helper functions

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   \file

*/

#pragma once

#include "common/common_pch.h"

#include "common/hevc/types.h"

namespace mtx::hevc {

class hevcc_c {
public:
  unsigned int m_configuration_version,
               m_general_profile_space,
               m_general_tier_flag,
               m_general_profile_idc,
               m_general_profile_compatibility_flag,
               m_general_progressive_source_flag,
               m_general_interlace_source_flag,
               m_general_nonpacked_constraint_flag,
               m_general_frame_only_constraint_flag,
               m_general_level_idc,
               m_min_spatial_segmentation_idc,
               m_parallelism_type,
               m_chroma_format_idc,
               m_bit_depth_luma_minus8,
               m_bit_depth_chroma_minus8,
               m_max_sub_layers,
               m_temporal_id_nesting_flag,
               m_size_nalu_minus_one,
               m_nalu_size_length;
  std::vector<memory_cptr> m_vps_list,
                           m_sps_list,
                           m_pps_list,
                           m_sei_list;
  std::vector<vps_info_t> m_vps_info_list;
  std::vector<sps_info_t> m_sps_info_list;
  std::vector<pps_info_t> m_pps_info_list;
  user_data_t m_user_data;
  codec_private_t m_codec_private;

public:
  hevcc_c();
  hevcc_c(unsigned int nalu_size_len,
          std::vector<memory_cptr> vps_list,
          std::vector<memory_cptr> sps_list,
          std::vector<memory_cptr> pps_list,
          user_data_t user_data,
          codec_private_t const &codec_private);

  explicit operator bool() const;

  bool parse_vps_list(bool ignore_errors = false);
  bool parse_sps_list(bool ignore_errors = false);
  bool parse_pps_list(bool ignore_errors = false);
  bool parse_sei_list();

  memory_cptr pack();
  static hevcc_c unpack(memory_cptr const &mem);
};

}                              // namespace mtx::hevc
