#pragma once

#include "common/common_pch.h"

#if defined(SYS_APPLE)

#include "mkvtoolnix-gui/jobs/program_runner.h"

namespace mtx { namespace gui { namespace Jobs {

class MacOSProgramRunner: public ProgramRunner {
  Q_OBJECT;

public:
  explicit MacOSProgramRunner();
  virtual ~MacOSProgramRunner();

  virtual QString defaultAudioFileName() const override;
};

}}}

#endif  // SYS_APPLE
