/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   helper functions, common variables

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <QDateTime>

#include "common/command_line.h"
#include "common/date_time.h"
#include "common/debugging.h"
#include "common/ebml.h"
#include "common/endian.h"
#include "common/fs_sys_helpers.h"
#include "common/json.h"
#include "common/locale.h"
#include "common/logger.h"
#include "common/mm_io.h"
#include "common/mm_stdio.h"
#include "common/strings/utf8.h"

bool g_suppress_info              = false;
bool g_suppress_warnings          = false;
bool g_warning_issued             = false;
std::string g_stdio_charset;
static bool s_mm_stdio_redirected = false;

charset_converter_cptr g_cc_stdio = std::make_shared<charset_converter_c>();
std::shared_ptr<mm_io_c> g_mm_stdio   = std::shared_ptr<mm_io_c>(new mm_stdio_c);

static mxmsg_handler_t s_mxmsg_info_handler, s_mxmsg_warning_handler, s_mxmsg_error_handler;
static std::vector<std::string> s_warnings_emitted, s_errors_emitted;

static nlohmann::json
to_json_array(std::vector<std::string> const &messages) {
  auto result = nlohmann::json::array();

  for (auto const &message : messages)
    result.push_back(message);

  return result;
}

void
display_json_output(nlohmann::json json) {
  json["warnings"] = to_json_array(s_warnings_emitted);
  json["errors"]   = to_json_array(s_errors_emitted);

  mxinfo(fmt::format("{0}\n", mtx::json::dump(json, 2)));
}

static void
json_warning_error_handler(unsigned int level,
                           std::string const &message) {
  if (MXMSG_WARNING == level) {
    s_warnings_emitted.push_back(message);

    if (mtx::cli::g_abort_on_warnings) {
      display_json_output(nlohmann::json{});
      mxexit(1);
    }

  } else {
    s_errors_emitted.push_back(message);
    display_json_output(nlohmann::json{});
    mxexit(2);
  }
}

void
redirect_warnings_and_errors_to_json() {
  set_mxmsg_handler(MXMSG_WARNING, json_warning_error_handler);
  set_mxmsg_handler(MXMSG_ERROR,   json_warning_error_handler);
}

void
redirect_stdio(const mm_io_cptr &stdio) {
  g_mm_stdio            = stdio;
  s_mm_stdio_redirected = true;
  g_mm_stdio->set_string_output_converter(g_cc_stdio);
}

bool
stdio_redirected() {
  return s_mm_stdio_redirected;
}

void
set_mxmsg_handler(unsigned int level,
                  mxmsg_handler_t const &handler) {
  if (MXMSG_INFO == level)
    s_mxmsg_info_handler = handler;
  else if (MXMSG_WARNING == level)
    s_mxmsg_warning_handler = handler;
  else if (MXMSG_ERROR == level)
    s_mxmsg_error_handler = handler;
  else
    assert(false);
}

void
mxmsg(unsigned int level,
      std::string message) {
  static debugging_option_c s_timestamped_messages{"timestamped_messages"};
  static debugging_option_c s_memory_usage_in_messages{"memory_usage_in_messages"};
  static bool s_saw_cr_after_nl = false;

  if (g_suppress_info && (MXMSG_INFO == level))
    return;

  if ('\n' == message[0]) {
    message.erase(0, 1);
    g_mm_stdio->puts("\n");
    s_saw_cr_after_nl = false;
  }

  std::string prefix;
  if (s_timestamped_messages) {
    prefix += mtx::date_time::format(QDateTime::currentDateTime(), "%Y-%m-%d %H:%M:%S.%f ");
  }
  if (s_memory_usage_in_messages) {
    prefix += fmt::format("{0} kB ", mtx::sys::get_memory_usage() / 1024);
  }

  if (level == MXMSG_ERROR) {
    if (s_saw_cr_after_nl)
      g_mm_stdio->puts("\n");
    if (balg::starts_with(message, Y("Error:")))
      message.erase(0, std::string{Y("Error:")}.length());
    g_mm_stdio->puts(mtx::cli::g_gui_mode ? "#GUI#error " : fmt::format("{0}{1} ", prefix, Y("Error:")));

  } else if (level == MXMSG_WARNING)
    g_mm_stdio->puts(mtx::cli::g_gui_mode ? "#GUI#warning " : fmt::format("{0}{1} ", prefix, Y("Warning:")));

  size_t idx_cr = message.rfind('\r');
  if (std::string::npos != idx_cr) {
    size_t idx_nl = message.rfind('\n');
    if ((std::string::npos != idx_nl) && (idx_nl < idx_cr))
      s_saw_cr_after_nl = true;
  }

  g_mm_stdio->puts(prefix.empty() ? message : (prefix + message));
  g_mm_stdio->flush();
}

static void
default_mxinfo(unsigned int,
               std::string const &info) {
  mxmsg(MXMSG_INFO, info);
}

void
mxinfo(std::string const &info) {
  if (s_mxmsg_info_handler)
    s_mxmsg_info_handler(MXMSG_INFO, info);
}

void
mxinfo(const std::wstring &info) {
  mxinfo(to_utf8(info));
}

static void
default_mxwarn(unsigned int,
               std::string const &warning) {
  if (g_suppress_warnings)
    return;

  mxmsg(MXMSG_WARNING, warning);

  if (mtx::cli::g_abort_on_warnings)
    mxexit(1);

  g_warning_issued = true;
}

void
mxwarn(std::string const &warning) {
  if (s_mxmsg_warning_handler)
    s_mxmsg_warning_handler(MXMSG_WARNING, warning);
}

static void
default_mxerror(unsigned int,
                std::string const &error) {
  mxmsg(MXMSG_ERROR, error);
  mxexit(2);
}

void
mxerror(std::string const &error) {
  if (s_mxmsg_error_handler)
    s_mxmsg_error_handler(MXMSG_ERROR, error);
}

void
mxinfo_fn(const std::string &file_name,
          const std::string &info) {
  mxinfo(fmt::format(FY("'{0}': {1}"), file_name, info));
}

void
mxinfo_tid(const std::string &file_name,
           int64_t track_id,
           const std::string &info) {
  mxinfo(fmt::format(FY("'{0}' track {1}: {2}"), file_name, track_id, info));
}

void
mxwarn_fn(const std::string &file_name,
          const std::string &warning) {
  mxwarn(fmt::format(FY("'{0}': {1}"), file_name, warning));
}

void
mxwarn_tid(const std::string &file_name,
           int64_t track_id,
           const std::string &warning) {
  mxwarn(fmt::format(FY("'{0}' track {1}: {2}"), file_name, track_id, warning));
}

void
mxerror_fn(const std::string &file_name,
           const std::string &error) {
  mxerror(fmt::format(FY("'{0}': {1}"), file_name, error));
}

void
mxerror_tid(const std::string &file_name,
            int64_t track_id,
            const std::string &error) {
  mxerror(fmt::format(FY("'{0}' track {1}: {2}"), file_name, track_id, error));
}

void
init_common_output(bool no_charset_detection) {
  if (no_charset_detection)
    set_cc_stdio("UTF-8");
  else
#if defined(SYS_WINDOWS)
    set_cc_stdio("UTF-8");
#else
    set_cc_stdio(get_local_console_charset());
#endif
  set_mxmsg_handler(MXMSG_INFO,    default_mxinfo);
  set_mxmsg_handler(MXMSG_WARNING, default_mxwarn);
  set_mxmsg_handler(MXMSG_ERROR,   default_mxerror);
}

void
set_cc_stdio(const std::string &charset) {
  g_stdio_charset = charset;
  g_cc_stdio      = charset_converter_c::init(charset);
  g_mm_stdio->set_string_output_converter(g_cc_stdio);
}
