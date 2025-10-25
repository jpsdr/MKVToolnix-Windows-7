/*
  mkvmerge -- utility for splicing together matroska files
  from component media subtypes

  Distributed under the GPL v2
  see the file COPYING for details
  or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

  Blu-ray helper functions

  Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx::bluray {

boost::filesystem::path find_base_dir(boost::filesystem::path const &file_name);
boost::filesystem::path find_other_file(boost::filesystem::path const &reference_file_name, boost::filesystem::path const &other_file_name);

}
