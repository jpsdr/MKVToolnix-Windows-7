/*
  mkvmerge -- utility for splicing together matroska files
  from component media subtypes

  Distributed under the GPL v2
  see the file COPYING for details
  or visit http://www.gnu.org/copyleft/gpl.html

  definitions and helper functions for PNG image handling

  Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

class mm_io_c;

namespace mtx::image::png {

boost::optional<std::pair<unsigned int, unsigned int>> get_size(bfs::path const &file_name);
boost::optional<std::pair<unsigned int, unsigned int>> get_size(mm_io_c &file);

} // namespace mtx::image::png
