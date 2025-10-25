/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   IANA language subtag registry

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx::bcp47 {
class language_c;
}

namespace mtx::iana::language_subtag_registry {

struct entry_t {
  std::string const code, description;
  std::vector<std::string> prefixes;
  bool is_deprecated;

  entry_t(std::string &&p_code, std::string &&p_description, bool p_is_deprecated)
    : code{std::move(p_code)}
    , description{std::move(p_description)}
    , is_deprecated{p_is_deprecated}
  {
  }
};

extern std::vector<entry_t> g_extlangs, g_variants, g_grandfathered;
extern std::vector< std::pair<mtx::bcp47::language_c, mtx::bcp47::language_c> > g_preferred_values;
extern std::unordered_map<std::string, std::string> g_suppress_scripts;

void init();
void init_preferred_values();
std::optional<entry_t> look_up_extlang(std::string const &s);
std::optional<entry_t> look_up_variant(std::string const &s);
std::optional<entry_t> look_up_grandfathered(std::string const &s);

} // namespace mtx::iana::language_subtag_registry
