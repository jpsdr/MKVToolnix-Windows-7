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

  return mtx::included_in(type, Util::Settings::RunProgramType::ShutDownComputer, Util::Settings::RunProgramType::HibernateComputer, Util::Settings::RunProgramType::SleepComputer);
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
LinuxProgramRunner::hibernateOrSleepComputer(bool hibernate) {
  auto action = Q(hibernate ? "hibernate" : "suspend");
  qDebug() << "LinuxProgramRunner::hibernateOrSleepComputer: about to" << action << "the computer via systemctl";

  auto result = QProcess::execute(Q("systemctl %1").arg(action));

  if (result != 0)
    qDebug() << "LinuxProgramRunner::hibernateOrSleepComputer: 'systemctl" << action << "' failed:" << result;
}

void
LinuxProgramRunner::hibernateComputer(Util::Settings::RunProgramConfig &) {
  hibernateOrSleepComputer(true);
}

void
LinuxProgramRunner::sleepComputer(Util::Settings::RunProgramConfig &) {
  hibernateOrSleepComputer(false);
}

}}}

#endif  // SYS_LINUX
