/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   ISO639 language definitions, lookup functions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

struct iso639_language_t {
  std::string const english_name, iso639_2_code, iso639_1_code, terminology_abbrev;
};

extern std::vector<iso639_language_t> const g_iso639_languages;
extern std::vector<std::string> const g_popular_language_codes;

int map_to_iso639_2_code(std::string const &s, bool allow_short_english_names = false);
bool is_valid_iso639_2_code(std::string const &s);
std::string const &map_iso639_2_to_iso639_1(std::string const &iso639_2_code);
void list_iso639_languages();
bool is_popular_language(std::string const &lang);
bool is_popular_language_code(std::string const &code);
