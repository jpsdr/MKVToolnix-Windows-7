/*
   mkvpropedit -- utility for editing properties of existing Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/logger.h"

namespace mtx::log {

class windows_debug_target_c: public target_c {
public:
  windows_debug_target_c();
  virtual ~windows_debug_target_c();

protected:
  virtual void log_line(std::string const &message) override;

public:
  static void activate();
};

}
