#include "common/common_pch.h"

#if defined(SYS_LINUX)

#include <QDebug>
#include <QProcess>

#include "common/list_utils.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/jobs/program_runner/linux_program_runner.h"

namespace mtx { namespace gui { namespace Jobs {

LinuxProgramRunner::LinuxProgramRunner()
  : ProgramRunner{}
{
}

LinuxProgramRunner::~LinuxProgramRunner() {
}

bool
LinuxProgramRunner::isRunProgramTypeSupported(Util::Settings::RunProgramType type) {
  if (ProgramRunner::isRunProgramTypeSupported(type))
    return true;

  return mtx::included_in(type, Util::Settings::RunProgramType::ShutDownComputer, Util::Settings::RunProgramType::SuspendComputer);
}

void
LinuxProgramRunner::shutDownComputer(Util::Settings::RunProgramConfig &) {
  qDebug() << "LinuxProgramRunner::shutDownComputer: about to shut down the computer via systemctl";

  auto result = QProcess::execute("systemctl poweroff");

  if (result == 0)
    return;

  qDebug() << "LinuxProgramRunner::shutDownComputer: 'systemctl poweroff' failed:" << result;
}

void
LinuxProgramRunner::suspendComputer(Util::Settings::RunProgramConfig &) {
  qDebug() << "LinuxProgramRunner::suspendComputer: about to shut down the computer via systemctl";

  auto result = QProcess::execute("systemctl suspend");

  if (result == 0)
    return;

  qDebug() << "LinuxProgramRunner::suspendComputer: 'systemctl poweroff' failed:" << result;
}

}}}

#endif  // SYS_LINUX
