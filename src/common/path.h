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

#include <QString>

namespace mtx::fs {

std::filesystem::path to_path(std::string const &name);
std::filesystem::path to_path(std::wstring const &name);

inline std::filesystem::path
to_path(char const *name) {
  return to_path(std::string{name});
}

inline std::filesystem::path
to_path(QString const &name) {
  return to_path(name.toStdWString());
}

// Compatibility functions due to bugs in gcc/libstdc++ on Windows:
bool is_absolute(std::filesystem::path const &p);
std::filesystem::path absolute(std::filesystem::path const &p);
void create_directories(std::filesystem::path const &path, std::error_code &error_code);

} // namespace mtx::fs

#if FMT_VERSION >= 90000
template <> struct fmt::formatter<std::filesystem::path> : ostream_formatter {};
#endif  // FMT_VERSION >= 90000
