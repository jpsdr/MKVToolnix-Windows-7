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

namespace mtx::iana::language_subtag_registry {

struct entry_t {
  std::string const code, description;
  std::vector<std::string> const prefixes;
};

extern std::vector<entry_t> const g_extlangs, g_variants;

std::optional<std::size_t> look_up_extlang(std::string const &s);
std::optional<std::size_t> look_up_variant(std::string const &s);

} // namespace mtx::iana::language_subtag_registry
