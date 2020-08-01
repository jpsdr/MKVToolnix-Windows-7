/*
  mkvmerge -- utility for splicing together matroska files
  from component media subtypes

  Distributed under the GPL v2
  see the file COPYING for details
  or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

  Blu-ray utility functions

  Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/bluray/util.h"

namespace mtx::bluray {

namespace {

bfs::path
find_base_dir_impl(bfs::path const &file_name) {
  auto dir = bfs::absolute(file_name);
  if (!bfs::is_directory(dir))
    dir = dir.parent_path();

  while (!dir.empty()) {
    if (   bfs::is_regular_file(dir / "index.bdmv")
        && bfs::is_directory(dir / "STREAM")
        && bfs::is_directory(dir / "PLAYLIST"))
      return dir;

    dir = dir.parent_path();
  }

  return {};
}

bfs::path
find_other_file_impl(bfs::path const &reference_file_name,
                     bfs::path const &other_file_name) {
  auto base_dir = find_base_dir(reference_file_name);
  if (base_dir.empty())
    return {};

  auto file_name = base_dir / other_file_name;
  if (bfs::exists(file_name))
    return file_name;

  return {};
}

} // anonymous namespace

bfs::path
find_base_dir(bfs::path const &file_name) {
  try {
    return find_base_dir_impl(file_name);
  } catch (boost::filesystem::filesystem_error &) {
    return {};
  }
}

bfs::path
find_other_file(bfs::path const &reference_file_name,
                bfs::path const &other_file_name) {
  try {
    return find_other_file_impl(reference_file_name, other_file_name);
  } catch (boost::filesystem::filesystem_error &) {
    return {};
  }
}

}
