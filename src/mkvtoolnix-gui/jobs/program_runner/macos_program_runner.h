#ifndef MTX_MKVTOOLNIX_GUI_JOBS_PROGRAM_RUNNER_MACOS_PROGRAM_RUNNER_H
#define MTX_MKVTOOLNIX_GUI_JOBS_PROGRAM_RUNNER_MACOS_PROGRAM_RUNNER_H

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
#endif  // MTX_MKVTOOLNIX_GUI_JOBS_PROGRAM_RUNNER_MACOS_PROGRAM_RUNNER_H
