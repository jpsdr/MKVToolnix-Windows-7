/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/path.h"
#include "common/strings/utf8.h"

namespace mtx::fs {

std::filesystem::path
to_path(std::string const &name) {
  return std::filesystem::path{to_wide(name)};
}

std::filesystem::path
to_path(std::wstring const &name) {
  return std::filesystem::path{name};
}

bool
is_absolute(std::filesystem::path const &p) {
  auto p_s = p.u8string();

  if (   (p_s.substr(0, 2) == "//"s)
      || (p_s.substr(0, 2) == "\\\\"s))
    return true;

  return p.is_absolute();
}

void
create_directories(std::filesystem::path const &path,
                   std::error_code &error_code) {
  auto normalized_path = path.u8string();

  for (auto &c : normalized_path)
    if (c == '/')
      c = '\\';

  auto directory = mtx::fs::to_path(normalized_path);
  error_code     = {};

  if (directory.empty() || std::filesystem::is_directory(directory))
    return;

  auto is_unc = normalized_path.substr(0, 2) == "\\\\"s;

  std::vector<std::filesystem::path> to_check;

  // path                   | is_unc | parent_path        | parent_path.parent_path | to_check
  // -----------------------+--------+--------------------+-------------------------+----------------------
  // E:\moo\cow             | false  | E:\moo             | E:\                     | E:\moo\cow
  // E:\moo                 | false  | E:\                | E:\                     | E:\moo
  // E:\                    | false  | E:\                | E:\                     |
  // \\server\share\moo\cow | true   | \\server\share\moo | \\server\share          | \\sever\share\moo\cow
  // \\server\share\moo     | true   | \\server\share     | \\server                | \\sever\share\moo
  // \\server\share         | true   | \\server           | \                       |
  // \\server               | true   | \                  | \                       |

  while (   !directory.empty()
         && (directory.parent_path() != directory)
         && (   !is_unc
             || (directory.parent_path().parent_path().u8string() != "\\"s))) {
    to_check.emplace_back(directory);
    directory = directory.parent_path();
  }

  for (auto itr = to_check.rbegin(), end = to_check.rend(); itr != end; ++itr) {
    if (std::filesystem::is_directory(*itr))
      continue;

    std::filesystem::create_directory(*itr, error_code);

    if (error_code)
      return;
  }
}

} // namespace mtx::fs
