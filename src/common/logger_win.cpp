/*
   mkvpropedit -- utility for editing properties of existing Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#if defined(SYS_WINDOWS)

#include <windows.h>

#include "common/logger_win.h"

windows_debug_logger_c::windows_debug_logger_c()
  : logger_c({})
{
}

windows_debug_logger_c::~windows_debug_logger_c() {
}

void
windows_debug_logger_c::log_line(std::string const &message) {
  auto line = to_wide((boost::format("+%1%ms %2%") % runtime() % message).str());
  OutputDebugStringW(line.c_str());
}

void
windows_debug_logger_c::activate() {
  s_default_logger = std::static_pointer_cast<logger_c>(std::make_shared<windows_debug_logger_c>());
}

#endif // SYS_WINDOWS
