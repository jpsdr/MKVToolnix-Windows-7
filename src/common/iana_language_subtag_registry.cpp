/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   IANA language subtag registry

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/iana_language_subtag_registry.h"
#include "common/strings/formatting.h"

namespace mtx::iana::language_subtag_registry {

std::optional<std::size_t>
look_up_variant(std::string const &s) {
  if (s.empty())
    return {};

  auto s_lower = mtx::string::to_lower_ascii(s);
  auto itr     = std::find_if(g_variants.begin(), g_variants.end(), [&s_lower](auto const &variant) {
    return s_lower == mtx::string::to_lower_ascii(variant.variant);
  });

  if (itr == g_variants.end())
    return {};

  return std::distance(g_variants.begin(), itr);
}

} // namespace mtx::iana::language_subtag_registry
