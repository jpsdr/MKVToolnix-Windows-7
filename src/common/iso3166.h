/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   ISO 3166 countries & UN M.49 regions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx::iso3166 {

struct region_t {
  std::string const alpha_2_code, alpha_3_code;
  unsigned int number;
  std::string const name, official_name;
};

extern std::vector<region_t> const g_regions;
extern std::vector<std::string> const g_popular_country_codes;

std::optional<region_t> look_up(std::string const &s);
std::optional<region_t> look_up(unsigned int number);

std::optional<region_t> look_up_cctld(std::string const &s);

} // namespace mtx::iso3166
