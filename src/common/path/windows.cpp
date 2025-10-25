/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/path.h"
#include "common/strings/utf8.h"

namespace mtx::fs {

boost::filesystem::path
to_path(std::string const &name) {
  return boost::filesystem::path{to_wide(name)};
}

boost::filesystem::path
to_path(std::wstring const &name) {
  return boost::filesystem::path{name};
}

} // namespace mtx::fs
