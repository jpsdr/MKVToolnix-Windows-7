/*
   mkvpropedit -- utility for editing properties of existing Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <chrono>
#include <ctime>
#include <iostream>

#include "common/logger.h"
#if defined(SYS_WINDOWS)
# include "common/logger_win.h"
#endif // SYS_WINDOWS
#include "common/fs_sys_helpers.h"
#include "common/mm_io_x.h"
#include "common/strings/formatting.h"

namespace mtx { namespace log {

target_cptr target_c::s_default_logger;

static auto s_program_start_time = std::chrono::system_clock::now();

target_c::target_c()
  : m_log_start{mtx::sys::get_current_time_millis()}
{
}

target_c::~target_c() {
}

std::string
target_c::format_line(std::string const &message) {
  auto now  = std::chrono::system_clock::now();
  auto diff = now - s_program_start_time;
  auto tnow = std::chrono::system_clock::to_time_t(now);

  // 2013-03-02 15:42:32
  char timestamp[30];
  std::strftime(timestamp, 30, "%Y-%m-%d %H:%M:%S", std::localtime(&tnow));

  auto line = (boost::format("[mtx] %1% +%2%ms %3%") % timestamp % std::chrono::duration_cast<std::chrono::milliseconds>(diff).count() % message).str();
  if (message.size() && (message[message.size() - 1] != '\n'))
    line += "\n";

  return line;
}

target_c &
target_c::get_default_logger() {
  if (!s_default_logger) {
    auto var = mtx::sys::get_environment_variable("MTX_LOGGER");

    if (var.empty()) {
#if defined(SYS_WINDOWS)
      var = "debug";
#else
      var = "stderr";
#endif // SYS_WINDOWS
    }

    auto spec = split(var, ":");

    if (spec[0] == "file") {
      auto file = spec[1];
      if (file.empty())
        file = "mkvtoolnix-debug.log";

      set_default_logger(target_cptr{new file_target_c{file}});

#if defined(SYS_WINDOWS)
    } else if (spec[0] == "debug") {
      set_default_logger(target_cptr{new windows_debug_target_c{}});
#endif // SYS_WINDOWS

    } else
      set_default_logger(target_cptr{new stderr_target_c{}});
}
  return *s_default_logger;
}

void
target_c::set_default_logger(target_cptr const &logger) {
  s_default_logger = logger;
}

int64_t
target_c::runtime() {
  auto diff = std::chrono::system_clock::now() - s_program_start_time;
  return std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();
}

// ----------------------------------------------------------------------

file_target_c::file_target_c(bfs::path const &file_name)
  : target_c()                  // Don't use initializer-list syntax due to a bug in gcc < 4.8
  , m_file_name{file_name}
{
  if (!m_file_name.is_absolute())
    m_file_name = bfs::temp_directory_path() / m_file_name;

  if (bfs::exists(m_file_name)) {
    boost::system::error_code ec;
    bfs::remove(m_file_name, ec);
  }
}

file_target_c::~file_target_c() {
}

void
file_target_c::log_line(std::string const &message) {
  try {
    mm_text_io_c out(new mm_file_io_c(m_file_name.string(), bfs::exists(m_file_name) ? MODE_WRITE : MODE_CREATE));
    out.setFilePointer(0, seek_end);
    out.puts(format_line(message));
  } catch (mtx::mm_io::exception &ex) {
  }
}

// ----------------------------------------------------------------------

stderr_target_c::stderr_target_c()
  : target_c()                  // Don't use initializer-list syntax due to a bug in gcc < 4.8
{
}

stderr_target_c::~stderr_target_c() {
}

void
stderr_target_c::log_line(std::string const &message) {
  std::cerr << message;
}

}}
