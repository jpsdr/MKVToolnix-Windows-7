/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   IANA language subtag registry

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx::iana::language_subtag_registry {

struct variant_t {
  std::string const variant, description;
  std::vector<std::string> const prefixes;
};

extern std::vector<variant_t> const g_variants;

std::optional<std::size_t> look_up_variant(std::string const &s);

} // namespace mtx::iana::language_subtag_registry
