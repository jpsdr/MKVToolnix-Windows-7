/** MPEG-4 p10 AVCC functions

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   \file

   \author Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx::avc {

struct sps_info_t;
struct pps_info_t;

class avcc_c {
public:
  unsigned int m_profile_idc, m_profile_compat, m_level_idc, m_nalu_size_length;
  std::vector<memory_cptr> m_sps_list, m_pps_list;
  std::vector<sps_info_t> m_sps_info_list;
  std::vector<pps_info_t> m_pps_info_list;
  memory_cptr m_trailer;

public:
  avcc_c();
  avcc_c(unsigned int nalu_size_len, std::vector<memory_cptr> sps_list, std::vector<memory_cptr> pps_list);

  explicit operator bool() const;

  memory_cptr pack();
  bool parse_sps_list(bool ignore_errors = false);
  bool parse_pps_list(bool ignore_errors = false);

  static avcc_c unpack(memory_cptr const &mem);
};

}
