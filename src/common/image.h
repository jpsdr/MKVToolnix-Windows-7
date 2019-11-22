/*
  mkvmerge -- utility for splicing together matroska files
  from component media subtypes

  Distributed under the GPL v2
  see the file COPYING for details
  or visit http://www.gnu.org/copyleft/gpl.html

  definitions and helper functions for image handling

  Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx::image {

boost::optional<std::pair<unsigned int, unsigned int>> get_size(bfs::path const &file_name);

}
