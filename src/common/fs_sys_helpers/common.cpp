/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   OS dependant file system & system helper functions

   Written by Moritz Bunkus <mo@bunkus.online>
*/

#include "common/common_pch.h"

#include "common/fs_sys_helpers.h"
#include "common/path.h"
#include "common/strings/parsing.h"

namespace mtx::sys {

namespace {

boost::filesystem::path s_current_executable_path;
std::unordered_map<std::string, boost::filesystem::path> s_exes_in_path;

boost::filesystem::path
find_exe_in_path_worker(boost::filesystem::path const &exe) {
  auto paths = mtx::string::split(get_environment_variable("PATH"), ":");

  for (auto const &path : paths) {
    auto potential_exe = mtx::fs::to_path(path) / exe;
    if (boost::filesystem::exists(potential_exe))
      return potential_exe;

    potential_exe += mtx::fs::to_path(".exe");
    if (boost::filesystem::exists(potential_exe))
      return potential_exe;
  }

  return {};
}

} // anonymous

boost::filesystem::path
get_installation_path() {
  return s_current_executable_path;
}

void
determine_path_to_current_executable(std::string const &argv0) {
  s_current_executable_path = get_current_exe_path(argv0);
}

boost::filesystem::path
find_exe_in_path(boost::filesystem::path const &exe) {
  auto const exe_str = exe.string();
  auto const itr     = s_exes_in_path.find(exe_str);

  if (itr == s_exes_in_path.end())
    s_exes_in_path[exe_str] = find_exe_in_path_worker(exe);

  return s_exes_in_path[exe_str];
}

}
