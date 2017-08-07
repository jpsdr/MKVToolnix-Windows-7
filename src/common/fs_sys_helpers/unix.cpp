/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   OS dependant file system & system helper functions

   Written by Moritz Bunkus <moritz@bunkus.org>
*/

#include "common/common_pch.h"

#if !defined(SYS_WINDOWS)

#include <stdlib.h>
#include <sys/time.h>

#if defined(SYS_APPLE)
# include <mach-o/dyld.h>
#endif

#include "common/error.h"
#include "common/fs_sys_helpers.h"
#include "common/strings/editing.h"
#include "common/strings/parsing.h"
#include "common/strings/utf8.h"

namespace mtx { namespace sys {

int64_t
get_current_time_millis() {
  struct timeval tv;
  if (0 != gettimeofday(&tv, nullptr))
    return -1;

  return (int64_t)tv.tv_sec * 1000 + (int64_t)tv.tv_usec / 1000;
}

bfs::path
get_application_data_folder() {
  auto home = getenv("HOME");
  if (!home)
    return bfs::path{};

  // If $HOME/.mkvtoolnix exists already then keep using it to avoid
  // losing existing user configuration.
  auto old_default_folder = bfs::path{home} / ".mkvtoolnix";
  if (boost::filesystem::exists(old_default_folder))
    return old_default_folder;

  // If XDG_CONFIG_HOME is set then use that folder.
  auto xdg_config_home = getenv("XDG_CONFIG_HOME");
  if (xdg_config_home)
    return bfs::path{xdg_config_home} / "mkvtoolnix";

  // If all fails then use the XDG fallback folder for config files.
  return bfs::path{home} / ".config" / "mkvtoolnix";
}

std::string
get_environment_variable(std::string const &key) {
  auto var = getenv(key.c_str());
  return var ? var : "";
}

void
unset_environment_variable(std::string const &key) {
  unsetenv(key.c_str());
}

int
system(std::string const &command) {
  return ::system(command.c_str());
}

bfs::path
get_current_exe_path(std::string const &argv0) {
#if defined(SYS_APPLE)
  std::string file_name;
  file_name.resize(4000);

  while (true) {
    memset(&file_name[0], 0, file_name.size());
    uint32_t size = file_name.size();
    auto result   = _NSGetExecutablePath(&file_name[0], &size);
    file_name.resize(size);

    if (0 == result)
      break;
  }

  return bfs::absolute(bfs::path{file_name}).parent_path();

#else // SYS_APPLE
  auto exe = bfs::path{"/proc/self/exe"};
  if (bfs::exists(exe)) {
    auto exe_path = bfs::read_symlink(exe);

    // only make absolute if needed, to avoid crash in case the current dir is deleted (as bfs::absolute calls bfs::current_path here)
    return (exe_path.is_absolute() ? exe_path : bfs::absolute(exe_path)).parent_path();
  }

  if (argv0.empty())
    return bfs::current_path();

  exe = bfs::absolute(argv0);
  if (bfs::exists(exe))
    return exe.parent_path();

  return bfs::current_path();
#endif
}

bool
is_installed() {
  return true;
}

uint64_t
get_memory_usage() {
  // This only works on Linux and other systems that implement a
  // Linux-compatible procfs on /proc. Not implemented for other
  // systems yet, as it's a debugging tool.

  try {
    auto content = mm_file_io_c::slurp("/proc/self/statm");
    if (!content) {
      return 0;
    }

    uint64_t value{};
    return parse_number(split(content->to_string(), " ", 2)[0], value) ? value * 4096 : 0;

  } catch (...) {
    return 0;
  }
}

}}

#endif  // !SYS_WINDOWS
