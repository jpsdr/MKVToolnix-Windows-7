#pragma once

#include "common/common_pch.h"

#if !defined(SYS_APPLE) && !defined(SYS_WINDOWS)

#include "mkvtoolnix-gui/jobs/program_runner.h"

namespace mtx::gui {

namespace Jobs {

class LinuxProgramRunner: public ProgramRunner {
  Q_OBJECT

public:
  explicit LinuxProgramRunner();
  virtual ~LinuxProgramRunner();

  virtual bool isRunProgramTypeSupported(Util::Settings::RunProgramType type) override;

protected:
  virtual void shutDownComputer(Util::Settings::RunProgramConfig &config) override;
  virtual void hibernateComputer(Util::Settings::RunProgramConfig &config) override;
  virtual void sleepComputer(Util::Settings::RunProgramConfig &config) override;
  virtual void showDesktopNotification(Util::Settings::RunProgramForEvent const forEvent, VariableMap const &variables) override;
  virtual void systemctlAction(QString const &action);
};

}}

#endif  // !SYS_APPLE && !SYS_WINDOWS
