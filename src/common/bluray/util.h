/*
  mkvmerge -- utility for splicing together matroska files
  from component media subtypes

  Distributed under the GPL v2
  see the file COPYING for details
  or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

  Blu-ray helper functions

  Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx::bluray {

std::filesystem::path find_base_dir(std::filesystem::path const &file_name);
std::filesystem::path find_other_file(std::filesystem::path const &reference_file_name, std::filesystem::path const &other_file_name);

}
