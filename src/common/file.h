/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   file helper functions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef MTX_COMMON_FILE_H
#define MTX_COMMON_FILE_H

#include "common/common_pch.h"

namespace mtx { namespace file {

bfs::path first_existing_path(std::vector<bfs::path> const &paths);

}}

#endif  // MTX_COMMON_FILE_H
