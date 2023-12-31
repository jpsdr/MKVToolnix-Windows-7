/** HEVC video helper functions

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   \file

*/

#pragma once

#include "common/common_pch.h"

#include "common/dovi_meta.h"
#include "common/hevc/types.h"

namespace mtx::hevc {

struct par_extraction_t {
  memory_cptr new_hevcc;
  unsigned int numerator, denominator;
  bool successful;

  bool is_valid() const;
};

bool parse_vps(memory_cptr const &buffer, vps_info_t &vps);
memory_cptr parse_sps(memory_cptr const &buffer, sps_info_t &sps, std::vector<vps_info_t> &vps_info_list, bool keep_ar_info = false);
bool parse_pps(memory_cptr const &buffer, pps_info_t &pps);
bool parse_sei(memory_cptr const &buffer, user_data_t &user_data);
bool handle_sei_payload(mm_mem_io_c &byte_reader, unsigned int sei_payload_type, unsigned int sei_payload_size, user_data_t &user_data);

bool parse_dovi_rpu(memory_cptr const &buffer, mtx::dovi::dovi_rpu_data_header_t &dovi_rpu_data_header);

par_extraction_t extract_par(memory_cptr const &buffer);
bool is_fourcc(const char *fourcc);
memory_cptr hevcc_to_nalus(const uint8_t *buffer, size_t size);

}                              // namespace mtx::hevc
