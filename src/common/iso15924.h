/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   ISO 15924 script codes

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx::iso15924 {

struct script_t {
  std::string const code;
  unsigned int number;
  std::string const english_name;
};

extern std::vector<script_t> const g_scripts;

std::optional<std::size_t> look_up(std::string const &s);

} // namespace mtx::iso15924
