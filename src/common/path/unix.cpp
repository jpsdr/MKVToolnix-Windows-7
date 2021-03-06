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
#include "common/strings/utf8.h"

namespace mtx::fs {

std::filesystem::path
to_path(std::string const &name) {
  return std::filesystem::path{name};
}

std::filesystem::path
to_path(std::wstring const &name) {
  return std::filesystem::path{to_utf8(name)};
}

bool
is_absolute(std::filesystem::path const &p) {
  return p.is_absolute();
}

} // namespace mtx::fs
