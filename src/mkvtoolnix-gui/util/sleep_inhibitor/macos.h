#pragma once

#include "common/common_pch.h"

#include "mkvtoolnix-gui/util/sleep_inhibitor.h"

namespace mtx::gui::Util {

class MacOSSleepInhibitorPrivate;
class MacOSSleepInhibitor : public BasicSleepInhibitor {
protected:
  MTX_DECLARE_PRIVATE(MacOSSleepInhibitorPrivate)

  explicit MacOSSleepInhibitor(MacOSSleepInhibitorPrivate &p);

public:
  MacOSSleepInhibitor();
  virtual ~MacOSSleepInhibitor();

  virtual bool inhibit() override;
  virtual void uninhibit() override;

  virtual bool isInhibited() const override;
};

}
