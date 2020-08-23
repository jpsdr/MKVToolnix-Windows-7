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

std::vector<std::string> const g_popular_country_codes{ "CN", "DE", "ES", "FI", "FR", "IT", "JP", "NL", "NO", "PT", "RU", "SE", "GB", "US" };

namespace {

std::vector<country_t> const s_cctlds_only{
  { "AC"s, {}, 0, u8"Ascension Island"s,     {} },
  { "AN"s, {}, 0, u8"Netherlands Antilles"s, {} },
  { "EU"s, {}, 0, u8"European Union"s,       {} },
  { "SU"s, {}, 0, u8"Soviet Union"s,         {} },
  { "UK"s, {}, 0, u8"United Kingdom"s,       {} },
};

std::map<std::string, std::string> const s_deprecated_cctlds{
  { "GB", "UK" },
  { "TP", "TL" },
};

std::optional<country_t>
look_up(std::function<bool(country_t const &)> const &test) {
  auto itr = std::find_if(g_countries.begin(), g_countries.end(), test);

  if (itr != g_countries.end())
    return *itr;

  return {};
}

} // anonymous namespace

std::optional<country_t>
look_up(std::string const &s) {
  if (s.empty())
    return {};

  auto s_upper = mtx::string::to_upper_ascii(s);

  return look_up([&s_upper](auto const &country) {
    return (s_upper == country.alpha_2_code) || (s_upper == country.alpha_3_code);
  });
}

std::optional<country_t>
look_up(unsigned int number) {
  return look_up([number](auto const &country) {
    return country.number == number;
  });
}

std::optional<country_t>
look_up_cctld(std::string const &s) {
  if (s.empty())
    return {};

  auto s_upper        = mtx::string::to_upper_ascii(s);
  auto deprecated_itr = s_deprecated_cctlds.find(s_upper);

  if (deprecated_itr != s_deprecated_cctlds.end())
    s_upper = deprecated_itr->second;

  auto cctld_itr = std::find_if(s_cctlds_only.begin(), s_cctlds_only.end(), [&s_upper](auto const &country) {
    return s_upper == country.alpha_2_code;
  });

  if (cctld_itr != s_cctlds_only.end())
    return *cctld_itr;

  return look_up([&s_upper](auto const &country) {
    return (s_upper == country.alpha_2_code) || (s_upper == country.alpha_3_code);
  });
}

} // namespace mtx::iso3166
