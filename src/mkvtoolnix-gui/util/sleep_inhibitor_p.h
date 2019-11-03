#pragma once

#include "common/common_pch.h"

#include "common/debugging.h"

namespace mtx::gui::Util {

class BasicSleepInhibitor;

class BasicSleepInhibitorPrivate {
public:
  std::vector<std::shared_ptr<BasicSleepInhibitor>> m_inhibitors;
  static debugging_option_c ms_debug;

  virtual ~BasicSleepInhibitorPrivate() = default;
};

}
