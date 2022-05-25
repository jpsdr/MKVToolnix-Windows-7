#include "common/common_pch.h"

#if defined(SYS_WINDOWS)

#include <QDebug>

#include <windows.h>
#include <powrprof.h>

#include "common/fs_sys_helpers.h"
#include "common/list_utils.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/jobs/program_runner/windows_program_runner.h"
#include "mkvtoolnix-gui/util/settings.h"

namespace mtx::gui::Jobs {

WindowsProgramRunner::WindowsProgramRunner()
  : ProgramRunner{}
{
}

WindowsProgramRunner::~WindowsProgramRunner() {
}

bool
WindowsProgramRunner::isRunProgramTypeSupported(Util::Settings::RunProgramType type) {
  if (ProgramRunner::isRunProgramTypeSupported(type))
    return true;

  return mtx::included_in(type, Util::Settings::RunProgramType::ShutDownComputer, Util::Settings::RunProgramType::HibernateComputer, Util::Settings::RunProgramType::SleepComputer);
}

void
WindowsProgramRunner::addShutdownNamePrivilege() {
  HANDLE hToken;
  TOKEN_PRIVILEGES tkp;

  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
    auto error = GetLastError();
    qDebug() << "WindowsProgramRunner::addPrivilege: OpenProcessToken failed:" << error << Q(mtx::sys::format_windows_message(error));
    return;
  }

  LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

  tkp.PrivilegeCount           = 1;
  tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

  AdjustTokenPrivileges(hToken, false, &tkp, 0, nullptr, 0);

  auto error = GetLastError();
  if (error != ERROR_SUCCESS)
    qDebug() << "WindowsProgramRunner::addPrivilege: AdjustTokenPrivileges failed:" << error << Q(mtx::sys::format_windows_message(error));
}

void
WindowsProgramRunner::shutDownComputer(Util::Settings::RunProgramConfig &) {
  qDebug() << "WindowsProgramRunner::shutDownComputer: about to shut down the computer";

  addShutdownNamePrivilege();

  if (ExitWindowsEx(EWX_POWEROFF | EWX_FORCEIFHUNG, SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED))
    return;

  auto error = GetLastError();
  qDebug() << "WindowsProgramRunner::shutDownComputer: failed, error code:" << error << Q(mtx::sys::format_windows_message(error));
}

void
WindowsProgramRunner::hibernateOrSleepComputer(bool hibernate) {
  auto action = Q(hibernate ? "hibernate" : "sleep");
  qDebug() << "WindowsProgramRunner::hibernateOrSleepComputer: about to" << action;

  addShutdownNamePrivilege();

  if (SetSuspendState(hibernate, false, false))
    return;

  auto error = GetLastError();
  qDebug() << "WindowsProgramRunner::hibernateOrSleepComputer:" << action << "failed; error:" << error << Q(mtx::sys::format_windows_message(error));
}

void
WindowsProgramRunner::hibernateComputer(Util::Settings::RunProgramConfig &) {
  hibernateOrSleepComputer(true);
}

void
WindowsProgramRunner::sleepComputer(Util::Settings::RunProgramConfig &) {
  hibernateOrSleepComputer(false);
}

QString
WindowsProgramRunner::defaultAudioFileName()
  const {
  return Q("<MTX_INSTALLATION_DIRECTORY>\\data\\sounds\\finished-1.webm");
}

}

#endif  // SYS_WINDOWS
