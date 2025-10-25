/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <QString>

namespace mtx::fs {

boost::filesystem::path to_path(std::string const &name);
boost::filesystem::path to_path(std::wstring const &name);

inline boost::filesystem::path
to_path(char const *name) {
  return to_path(std::string{name});
}

inline boost::filesystem::path
to_path(QString const &name) {
  return to_path(name.toStdWString());
}

} // namespace mtx::fs

#if FMT_VERSION >= 90000
template <> struct fmt::formatter<boost::filesystem::path> : ostream_formatter {};
#endif  // FMT_VERSION >= 90000
