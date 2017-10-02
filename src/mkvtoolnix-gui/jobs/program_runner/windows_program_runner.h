#pragma once

#include "common/common_pch.h"

#if defined(SYS_WINDOWS)

#include "mkvtoolnix-gui/jobs/program_runner.h"

namespace mtx { namespace gui { namespace Jobs {

class WindowsProgramRunner: public ProgramRunner {
  Q_OBJECT;

public:
  explicit WindowsProgramRunner();
  virtual ~WindowsProgramRunner();

  virtual bool isRunProgramTypeSupported(Util::Settings::RunProgramType type) override;

  virtual QString defaultAudioFileName() const override;

protected:
  virtual void shutDownComputer(Util::Settings::RunProgramConfig &config) override;
  virtual void hibernateComputer(Util::Settings::RunProgramConfig &config) override;
  virtual void sleepComputer(Util::Settings::RunProgramConfig &config) override;
  virtual void hibernateOrSleepComputer(bool hibernate);
  virtual void addShutdownNamePrivilege();
};

}}}

#endif  // SYS_WINDOWS
