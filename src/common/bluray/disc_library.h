/*
  mkvmerge -- utility for splicing together matroska files
  from component media subtypes

  Distributed under the GPL v2
  see the file COPYING for details
  or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

  definitions and helper functions for Blu-ray disc library meta data

  Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx::bluray::disc_library {

struct thumbnail_t {
  boost::filesystem::path m_file_name;
  unsigned int m_width, m_height;
};

struct info_t {
  std::string m_title;
  std::vector<thumbnail_t> m_thumbnails;

  void dump() const;
};

struct disc_library_t {
  std::unordered_map<std::string, info_t> m_infos_by_language;

  void dump() const;
};

std::optional<disc_library_t> locate_and_parse(boost::filesystem::path const &location);
std::optional<info_t> locate_and_parse_for_language(boost::filesystem::path const &location, std::string const &language);

} // namespace mtx::bluray::disc_library
