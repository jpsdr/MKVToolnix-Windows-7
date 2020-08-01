/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   ISO 3166 countries

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/iso3166.h"
#include "common/strings/formatting.h"

namespace mtx::iso3166 {

namespace {

std::optional<std::size_t>
look_up(std::function<bool(country_t const &)> const &test) {
  auto itr = std::find_if(g_countries.begin(), g_countries.end(), test);

  if (itr == g_countries.end())
    return {};

  return std::distance(g_countries.begin(), itr);
}

} // anonymous namespace

std::optional<std::size_t>
look_up(std::string const &s) {
  if (s.empty())
    return {};

  auto s_upper = mtx::string::to_upper_ascii(s);

  return look_up([&s_upper](auto const &country) {
    return (s_upper == country.alpha_2_code) || (s_upper == country.alpha_3_code);
  });
}

std::optional<std::size_t>
look_up(unsigned int number) {
  return look_up([number](auto const &country) {
    return country.number == number;
  });
}

} // namespace mtx::iso3166
