/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

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

extern std::vector<mime_type_t> const g_mime_types;

std::string guess_mime_type(std::string ext, bool is_file);
std::string primary_file_extension_for_mime_type(std::string const &mime_type);
