/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   IANA language subtag registry

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx::bcp47 {
class language_c;
}

namespace mtx::iana::language_subtag_registry {

struct entry_t {
  std::string const code, description;
  std::vector<std::string> const prefixes;

  entry_t(std::string &&p_code, std::string &&p_description, std::vector<std::string> &&p_prefixes)
    : code{std::move(p_code)}
    , description{std::move(p_description)}
    , prefixes{std::move(p_prefixes)}
  {
  }
};

extern std::vector<entry_t> g_extlangs, g_variants, g_grandfathered;
extern std::vector< std::pair<mtx::bcp47::language_c, mtx::bcp47::language_c> > g_preferred_values;

void init();
void init_preferred_values();
std::optional<entry_t> look_up_extlang(std::string const &s);
std::optional<entry_t> look_up_variant(std::string const &s);
std::optional<entry_t> look_up_grandfathered(std::string const &s);

} // namespace mtx::iana::language_subtag_registry
