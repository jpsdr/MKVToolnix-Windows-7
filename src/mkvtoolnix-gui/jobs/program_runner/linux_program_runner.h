#ifndef MTX_MKVTOOLNIX_GUI_JOBS_PROGRAM_RUNNER_LINUX_PROGRAM_RUNNER_H
#define MTX_MKVTOOLNIX_GUI_JOBS_PROGRAM_RUNNER_LINUX_PROGRAM_RUNNER_H

#include "common/common_pch.h"

#if defined(SYS_LINUX)

#include "mkvtoolnix-gui/jobs/program_runner.h"

namespace mtx { namespace gui {

namespace Jobs {

class LinuxProgramRunner: public ProgramRunner {
  Q_OBJECT;

public:
  explicit LinuxProgramRunner();
  virtual ~LinuxProgramRunner();

  virtual bool isRunProgramTypeSupported(Util::Settings::RunProgramType type) override;

protected:
  virtual void shutDownComputer(Util::Settings::RunProgramConfig &config) override;
  virtual void suspendComputer(Util::Settings::RunProgramConfig &config) override;
};

}}}

#endif  // SYS_LINUX
#endif  // MTX_MKVTOOLNIX_GUI_JOBS_PROGRAM_RUNNER_LINUX_PROGRAM_RUNNER_H
