/*
   mkvtoolnix - A set of programs for manipulating Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Cross platform helper functions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx { namespace sys {

int64_t get_current_time_millis();

int system(std::string const &command);

void determine_path_to_current_executable(std::string const &argv0);
bfs::path get_current_exe_path(std::string const &argv0);
bfs::path get_application_data_folder();
bfs::path get_installation_path();
uint64_t get_memory_usage();

bool is_installed();

std::string get_environment_variable(const std::string &key);
void unset_environment_variable(std::string const &key);

#if defined(SYS_WINDOWS)

void set_environment_variable(const std::string &key, const std::string &value);

#define WINDOWS_VERSION_UNKNOWN      0x00000000
#define WINDOWS_VERSION_2000         0x00050000
#define WINDOWS_VERSION_XP           0x00050001
#define WINDOWS_VERSION_SERVER2003   0x00050002
#define WINDOWS_VERSION_VISTA        0x00060000
#define WINDOWS_VERSION_SERVER2008   0x00060000
#define WINDOWS_VERSION_SERVER2008R2 0x00060001
#define WINDOWS_VERSION_7            0x00060001

unsigned int get_windows_version();
std::string format_windows_message(uint64_t message_id);

#endif

}}
