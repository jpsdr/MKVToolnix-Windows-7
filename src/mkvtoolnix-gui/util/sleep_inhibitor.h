#pragma once

#include "common/common_pch.h"

namespace mtx::gui::Util {

class BasicSleepInhibitorPrivate;
class BasicSleepInhibitor {
protected:
  MTX_DECLARE_PRIVATE(BasicSleepInhibitorPrivate)

  std::unique_ptr<BasicSleepInhibitorPrivate> const p_ptr;

  explicit BasicSleepInhibitor(BasicSleepInhibitorPrivate &p);

public:
  BasicSleepInhibitor();
  virtual ~BasicSleepInhibitor();

  virtual void addInhibitor(std::shared_ptr<BasicSleepInhibitor> const &inhibitor);

  virtual bool inhibit();
  virtual void uninhibit();

  virtual bool isInhibited() const;

public:
  static std::unique_ptr<BasicSleepInhibitor> create();
};

}
