#pragma once

#include "common/common_pch.h"

#include "mkvtoolnix-gui/util/sleep_inhibitor.h"

namespace mtx { namespace gui { namespace Util {

class LogindSleepInhibitorPrivate;
class LogindSleepInhibitor : public BasicSleepInhibitor {
protected:
  MTX_DECLARE_PRIVATE(LogindSleepInhibitorPrivate)

  explicit LogindSleepInhibitor(LogindSleepInhibitorPrivate &p);

public:
  LogindSleepInhibitor();
  virtual ~LogindSleepInhibitor();

  virtual bool inhibit() override;
  virtual void uninhibit() override;

  virtual bool isInhibited() const override;
};

}}}
