/*
  mkvmerge -- utility for splicing together matroska files
  from component media subtypes

  Distributed under the GPL v2
  see the file COPYING for details
  or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

  definitions and helper functions for PNG image handling

  Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

class mm_io_c;

namespace mtx::image::png {

std::optional<std::pair<unsigned int, unsigned int>> get_size(boost::filesystem::path const &file_name);
std::optional<std::pair<unsigned int, unsigned int>> get_size(mm_io_c &file);

} // namespace mtx::image::png
