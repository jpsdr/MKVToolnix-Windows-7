/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   IANA language subtag registry

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/iana_language_subtag_registry.h"
#include "common/strings/formatting.h"

namespace mtx::iana::language_subtag_registry {

namespace {

std::optional<entry_t>
look_up_entry(std::string const &s,
             std::vector<entry_t> const &entries) {
  if (s.empty())
    return {};

  auto s_lower = mtx::string::to_lower_ascii(s);
  auto itr     = std::find_if(entries.begin(), entries.end(), [&s_lower](auto const &entry) {
    return s_lower == mtx::string::to_lower_ascii(entry.code);
  });

  if (itr != entries.end())
    return *itr;

  return {};
}

}

std::optional<entry_t>
look_up_extlang(std::string const &s) {
  return look_up_entry(s, g_extlangs);
}

std::optional<entry_t>
look_up_variant(std::string const &s) {
  return look_up_entry(s, g_variants);
}

std::optional<entry_t>
look_up_grandfathered(std::string const &s) {
  return look_up_entry(s, g_grandfathered);
}

} // namespace mtx::iana::language_subtag_registry
