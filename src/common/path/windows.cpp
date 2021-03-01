/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/path.h"

namespace mtx::fs {

bool
is_absolute(std::filesystem::path const &p) {
  auto p_s = p.u8string();

  if (   (p_s.substr(0, 2) == "//"s)
      || (p_s.substr(0, 2) == "\\\\"s))
    return true;

  return p.is_absolute();
}

} // namespace mtx::fs
