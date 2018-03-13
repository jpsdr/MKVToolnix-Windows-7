#pragma once

#include "common/common_pch.h"

#include "mkvtoolnix-gui/util/kax_info.h"
#include "mkvtoolnix-gui/util/runnable.h"

namespace mtx { namespace gui { namespace Info {

class InitialScan : public Util::Runnable {
protected:
  Util::KaxInfo &m_info;
  Util::KaxInfo::ScanType m_type;

public:
  explicit InitialScan(Util::KaxInfo &info, Util::KaxInfo::ScanType type);
  ~InitialScan();

  virtual void run() override;
  virtual void abort() override;
};

}}}
