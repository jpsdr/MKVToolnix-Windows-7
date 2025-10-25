/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   ISO 3166 countries & UN M.49 regions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx::iso3166 {

struct region_t {
  std::string const alpha_2_code, alpha_3_code;
  unsigned int number;
  std::string const name, official_name;
  bool is_deprecated;

  region_t(std::string &&p_alpha_2_code, std::string &&p_alpha_3_code, unsigned int p_number, std::string &&p_name, std::string &&p_official_name, bool p_is_deprecated)
    : alpha_2_code{std::move(p_alpha_2_code)}
    , alpha_3_code{std::move(p_alpha_3_code)}
    , number{p_number}
    , name{std::move(p_name)}
    , official_name{std::move(p_official_name)}
    , is_deprecated{p_is_deprecated}
  {
  }
};

extern std::vector<region_t> g_regions;

void init();
std::optional<region_t> look_up(std::string const &s);
std::optional<region_t> look_up(unsigned int number);

std::optional<region_t> look_up_cctld(std::string const &s);

} // namespace mtx::iso3166
