#include "common/common_pch.h"

#if !defined(SYS_APPLE) && !defined(SYS_WINDOWS)

#include <QDebug>
#include <QProcess>

#include "common/list_utils.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/jobs/program_runner/linux_program_runner.h"

namespace mtx::gui::Jobs {

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
LinuxProgramRunner::systemctlAction(QString const &action) {
  qDebug() << Q("LinuxProgramRunner::systemctlAction: about to execute 'systemctl %1'").arg(action);

  auto args   = QStringList{} << action;
  auto result = QProcess::execute(Q("systemctl"), args);

  if (result == 0)
    return;

  qDebug() << Q("LinuxProgramRunner::systemctlAction: 'systemctl %1' failed: %2").arg(action).arg(result);
}

void
LinuxProgramRunner::shutDownComputer(Util::Settings::RunProgramConfig &) {
  systemctlAction(Q("poweroff"));
}

void
LinuxProgramRunner::hibernateComputer(Util::Settings::RunProgramConfig &) {
  systemctlAction(Q("hibernate"));
}

void
LinuxProgramRunner::sleepComputer(Util::Settings::RunProgramConfig &) {
  systemctlAction(Q("suspend"));
}

}

#endif  // !SYS_APPLE && !SYS_WINDOWS
