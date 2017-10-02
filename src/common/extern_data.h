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

struct mime_type_t {
  std::string const name;
  std::vector<std::string> const extensions;

  mime_type_t(std::string const &p_name, std::vector<std::string> const p_extensions)
    : name{p_name}
    , extensions{p_extensions}
  {
  }
};

struct cctld_t {
  std::string code, country;
};

extern std::vector<std::string> const sub_charsets, g_popular_character_sets;
extern std::vector<cctld_t> const g_cctlds;
extern std::vector<std::string> const g_popular_country_codes;
extern std::vector<mime_type_t> const mime_types;

std::string guess_mime_type(std::string ext, bool is_file);
std::string primary_file_extension_for_mime_type(std::string const &mime_type);
boost::optional<std::string> map_to_cctld(std::string const &s);
