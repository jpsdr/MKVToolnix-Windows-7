/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   ISO 639 language definitions, lookup functions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx::iso639 {

struct language_t {
  std::string const english_name, alpha_3_code, alpha_2_code, terminology_abbrev;
  bool is_part_of_iso639_2{};
};

extern std::vector<language_t> const g_languages;

std::optional<language_t> look_up(std::string const &s, bool allow_short_english_names = false);
void list_languages();

} // namespace mtx::iso639
