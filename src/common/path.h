/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx::fs {

std::filesystem::path to_path(std::string const &name);
std::filesystem::path to_path(std::wstring const &name);

// Compatibility functions due to bugs in gcc/libstdc++ on Windows:
bool is_absolute(std::filesystem::path const &p);
std::filesystem::path absolute(std::filesystem::path const &p);
void create_directories(std::filesystem::path const &path, std::error_code &error_code);

} // namespace mtx::fs
