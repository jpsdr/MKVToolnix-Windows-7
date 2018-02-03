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

namespace mtx { namespace mime {

struct type_t {
  std::string const name;
  std::vector<std::string> const extensions;
};

extern std::vector<type_t> const g_types;

std::string guess_type(std::string ext, bool is_file);
std::string primary_file_extension_for_type(std::string const &type);

}}
