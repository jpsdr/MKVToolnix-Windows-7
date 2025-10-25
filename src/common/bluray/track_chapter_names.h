/*
  mkvmerge -- utility for splicing together matroska files
  from component media subtypes

  Distributed under the GPL v2
  see the file COPYING for details
  or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

  definitions and helper functions for Blu-ray track/chapter names meta data

  Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx::bluray::track_chapter_names {

using chapter_names_t     = std::pair<std::string, std::vector<std::string>>;
using all_chapter_names_t = std::vector<chapter_names_t>;

all_chapter_names_t locate_and_parse_for_title(boost::filesystem::path const &location, std::string const &title_number);
void dump(all_chapter_names_t const &list);

} // namespace mtx::bluray::track_chapter_names
