/*
   mkvpropedit -- utility for editing properties of existing Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <ctime>
#include <iostream>

#include <QDateTime>

#include "common/date_time.h"
#include "common/logger.h"
#if defined(SYS_WINDOWS)
# include "common/logger/windows.h"
#endif // SYS_WINDOWS
#include "common/fs_sys_helpers.h"
#include "common/mm_io_x.h"
#include "common/mm_file_io.h"
#include "common/mm_proxy_io.h"
#include "common/mm_text_io.h"
#include "common/path.h"
#include "common/strings/formatting.h"

namespace mtx::log {

target_cptr target_c::s_default_logger;

static QDateTime s_program_start_time;

target_c::target_c()
  : m_log_start{mtx::sys::get_current_time_millis()}
{
}

std::string
target_c::format_line(std::string const &message) {
  auto timestamp = mtx::date_time::format(QDateTime::currentDateTime(), "%Y-%m-%d %H:%M:%S");
  auto line      = fmt::format("[mtx] {0} +{1}ms {2}", timestamp, runtime(), message);

  if (message.size() && (message[message.size() - 1] != '\n'))
    line += "\n";

  return line;
}

target_c &
target_c::get_default_logger() {
  if (s_default_logger)
    return *s_default_logger;

  auto var = mtx::sys::get_environment_variable("MTX_LOGGER");

  if (var.empty()) {
#if defined(SYS_WINDOWS)
    var = "debug";
#else
    var = "stderr";
#endif // SYS_WINDOWS
  }

  auto spec = mtx::string::split(var, ":", 2);

  if (spec[0] == "file") {
    auto file = (spec.size() > 1) && !spec[1].empty() ? spec[1] : "mkvtoolnix-debug.txt"s;

    set_default_logger(target_cptr{new file_target_c{mtx::fs::to_path(file)}});

#if defined(SYS_WINDOWS)
  } else if (spec[0] == "debug") {
    set_default_logger(target_cptr{new windows_debug_target_c{}});
#endif // SYS_WINDOWS

  } else
    set_default_logger(target_cptr{new stderr_target_c{}});

  return *s_default_logger;
}

void
target_c::set_default_logger(target_cptr const &logger) {
  s_default_logger = logger;
}

int64_t
target_c::runtime() {
  return s_program_start_time.msecsTo(QDateTime::currentDateTime());
}

// ----------------------------------------------------------------------

file_target_c::file_target_c(boost::filesystem::path file_name)
  : target_c{}
  , m_file_name{std::move(file_name)}
{
  if (!m_file_name.is_absolute())
    m_file_name = boost::filesystem::temp_directory_path() / m_file_name;

  if (boost::filesystem::is_regular_file(m_file_name)) {
    boost::system::error_code ec;
    boost::filesystem::remove(m_file_name, ec);
  }
}

void
file_target_c::log_line(std::string const &message) {
  try {
    mm_text_io_c out(std::make_shared<mm_file_io_c>(m_file_name.string(), boost::filesystem::is_regular_file(m_file_name) ? libebml::MODE_WRITE : libebml::MODE_CREATE));
    out.setFilePointer(0, libebml::seek_end);
    out.puts(format_line(message));
  } catch (mtx::mm_io::exception &ex) {
  }
}

// ----------------------------------------------------------------------

stderr_target_c::stderr_target_c()
  : target_c{}
{
}

void
stderr_target_c::log_line(std::string const &message) {
  std::cerr << fmt::format("[mtx] +{0}ms {1}\n", runtime(), message);
}

// ----------------------------------------------------------------------

lifetime_logger_c::lifetime_logger_c(std::string const &comment)
  : m_comment{comment}
  , m_start{mtx::sys::get_current_time_millis()}
{
  target_c::get_default_logger() << fmt::format("lifetime log start for {0}\n", m_comment);
}

lifetime_logger_c::~lifetime_logger_c() {
  target_c::get_default_logger() << fmt::format("lifetime log runtime +{0}ms for {1}\n", mtx::sys::get_current_time_millis() - m_start, m_comment);
}

// ----------------------------------------------------------------------

void
init() {
  s_program_start_time = QDateTime::currentDateTime();
}

}
