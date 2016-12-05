/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   file helper functions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/file.h"

namespace mtx { namespace file {

bfs::path
first_existing_path(std::vector<bfs::path> const &paths) {
  for (auto const &path : paths)
    if (bfs::exists(path))
      return path;

  return {};
}

}}
