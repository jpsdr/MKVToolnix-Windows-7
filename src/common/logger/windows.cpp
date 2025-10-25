/*
   mkvpropedit -- utility for editing properties of existing Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <windows.h>

#include "common/logger/windows.h"

namespace mtx::log {

windows_debug_target_c::windows_debug_target_c()
  : target_c{}
{
}

windows_debug_target_c::~windows_debug_target_c() {
}

void
windows_debug_target_c::log_line(std::string const &message) {
  auto line = to_wide(fmt::format("[mtx] +{0}ms {1}", runtime(), message));
  OutputDebugStringW(line.c_str());
}

void
windows_debug_target_c::activate() {
  s_default_logger = std::static_pointer_cast<target_c>(std::make_shared<windows_debug_target_c>());
}

}
