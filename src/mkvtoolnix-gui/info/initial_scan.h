#pragma once

#include "common/common_pch.h"

#include "mkvtoolnix-gui/util/kax_info.h"
#include "mkvtoolnix-gui/util/runnable.h"

namespace mtx { namespace gui { namespace Info {

class InitialScan : public Util::Runnable {
protected:
  Util::KaxInfo &m_info;

public:
  explicit InitialScan(Util::KaxInfo &info);
  ~InitialScan();

  virtual void run() override;
  virtual void abort() override;
};

}}}
