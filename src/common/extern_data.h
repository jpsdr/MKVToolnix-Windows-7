/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   declarations for ComboBox data

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

struct cctld_t {
  std::string code, country;
};

extern std::vector<std::string> const sub_charsets, g_popular_character_sets;
extern std::vector<cctld_t> const g_cctlds;
extern std::vector<std::string> const g_popular_country_codes;

std::optional<std::string> map_to_cctld(std::string const &s);
