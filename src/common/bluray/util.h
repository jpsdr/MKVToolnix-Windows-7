/*
  mkvmerge -- utility for splicing together matroska files
  from component media subtypes

  Distributed under the GPL v2
  see the file COPYING for details
  or visit http://www.gnu.org/copyleft/gpl.html

  Blu-ray helper functions

  Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx::bluray {

bfs::path find_base_dir(bfs::path const &file_name);
bfs::path find_other_file(bfs::path const &reference_file_name, bfs::path const &other_file_name);

}
