/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   ISO 3166 countries

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx::iso3166 {

struct country_t {
  std::string const alpha_2_code, alpha_3_code;
  unsigned int number;
  std::string const name, official_name;
};

extern std::vector<country_t> const g_countries;

std::optional<std::size_t> look_up(std::string const &s);
std::optional<std::size_t> look_up(unsigned int number);

} // namespace mtx::iso3166
