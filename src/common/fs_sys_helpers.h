/*
   mkvtoolnix - A set of programs for manipulating Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Cross platform helper functions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx::sys {

void set_process_priority(int priority);

int64_t get_current_time_millis();

int system(std::string const &command);

void determine_path_to_current_executable(std::string const &argv0);
boost::filesystem::path get_current_exe_path(std::string const &argv0);
boost::filesystem::path get_application_data_folder();
boost::filesystem::path get_package_data_folder();
boost::filesystem::path get_installation_path();
boost::filesystem::path find_exe_in_path(boost::filesystem::path const &exe);
uint64_t get_memory_usage();

bool is_installed();

std::string get_environment_variable(const std::string &key);
void unset_environment_variable(std::string const &key);

#if defined(SYS_WINDOWS)

void set_environment_variable(const std::string &key, const std::string &value);

constexpr auto WINDOWS_VERSION_UNKNOWN      = 0x00000000;
constexpr auto WINDOWS_VERSION_2000         = 0x00050000;
constexpr auto WINDOWS_VERSION_XP           = 0x00050001;
constexpr auto WINDOWS_VERSION_SERVER2003   = 0x00050002;
constexpr auto WINDOWS_VERSION_VISTA        = 0x00060000;
constexpr auto WINDOWS_VERSION_SERVER2008   = 0x00060000;
constexpr auto WINDOWS_VERSION_SERVER2008R2 = 0x00060001;
constexpr auto WINDOWS_VERSION_7            = 0x00060001;

unsigned int get_windows_version();
std::string format_windows_message(uint64_t message_id);

bool is_high_contrast_enabled();

#endif  // SYS_WINDOWS

#if defined(SYS_APPLE)
enum class unicode_normalization_form_e {
  c,
  d,
};

std::string normalize_unicode_string(std::string const &src, unicode_normalization_form_e form);
#endif  // SYS_APPLE

}
