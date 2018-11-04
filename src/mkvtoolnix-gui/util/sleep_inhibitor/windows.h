#pragma once

#include "common/common_pch.h"

#include "mkvtoolnix-gui/util/sleep_inhibitor.h"

namespace mtx { namespace gui { namespace Util {

class WindowsSleepInhibitorPrivate;
class WindowsSleepInhibitor : public BasicSleepInhibitor {
protected:
  MTX_DECLARE_PRIVATE(WindowsSleepInhibitorPrivate);

  explicit WindowsSleepInhibitor(WindowsSleepInhibitorPrivate &p);

public:
  WindowsSleepInhibitor();
  virtual ~WindowsSleepInhibitor();

  virtual bool inhibit() override;
  virtual void uninhibit() override;

  virtual bool isInhibited() const override;
};

}}}
