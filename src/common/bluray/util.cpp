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
#include "common/debugging.h"

namespace mtx::bluray {

namespace {

std::filesystem::path
find_base_dir_impl(std::filesystem::path const &file_name) {
  static debugging_option_c s_debug{"bluray_find_base_dir"};

  auto dir = std::filesystem::absolute(file_name);
  if (!std::filesystem::is_directory(dir))
    dir = dir.parent_path();

  mxdebug_if(s_debug, fmt::format("mtx::bluray::find_base_dir_impl: file_name {0} dir {1}\n", file_name.u8string(), dir.u8string()));

  while (!dir.empty()) {
    mxdebug_if(s_debug, fmt::format("mtx::bluray::find_base_dir_impl:   checking {0} is_regular(index.bdmv) {1} is_directory(STREAM) {2} is_directory(PLAYLIST) {3}\n",
                                    dir.u8string(), std::filesystem::is_regular_file(dir / "index.bdmv"), std::filesystem::is_directory(dir / "STREAM"), std::filesystem::is_directory(dir / "PLAYLIST")));

    if (   std::filesystem::is_regular_file(dir / "index.bdmv")
        && std::filesystem::is_directory(dir / "STREAM")
        && std::filesystem::is_directory(dir / "PLAYLIST"))
      return dir;

    auto parent_path = dir.parent_path();
    if (parent_path.empty() || (parent_path == dir))
      break;

    dir = parent_path;
  }

  return {};
}

std::filesystem::path
find_other_file_impl(std::filesystem::path const &reference_file_name,
                     std::filesystem::path const &other_file_name) {
  auto base_dir = find_base_dir(reference_file_name);
  if (base_dir.empty())
    return {};

  auto file_name = base_dir / other_file_name;
  if (std::filesystem::exists(file_name))
    return file_name;

  return {};
}

} // anonymous namespace

std::filesystem::path
find_base_dir(std::filesystem::path const &file_name) {
  try {
    return find_base_dir_impl(file_name);
  } catch (std::filesystem::filesystem_error &) {
    return {};
  }
}

std::filesystem::path
find_other_file(std::filesystem::path const &reference_file_name,
                std::filesystem::path const &other_file_name) {
  try {
    return find_other_file_impl(reference_file_name, other_file_name);
  } catch (std::filesystem::filesystem_error &) {
    return {};
  }
}

}
