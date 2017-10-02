/*
   mkvpropedit -- utility for editing properties of existing Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#if defined(SYS_WINDOWS)

#include "common/logger.h"

namespace mtx { namespace log {

class windows_debug_target_c: public target_c {
public:
  windows_debug_target_c();
  virtual ~windows_debug_target_c();

protected:
  virtual void log_line(std::string const &message) override;

public:
  static void activate();
};

}}

#endif // SYS_WINDOWS
