#include "common/common_pch.h"

#if !defined(SYS_APPLE) && !defined(SYS_WINDOWS)

#if defined(HAVE_QTDBUS)
# include <QDBusArgument>
# include <QDBusConnection>
# include <QDBusInterface>
# include <QDBusReply>
#endif
#include <QDebug>
#include <QProcess>

#include "common/fs_sys_helpers.h"
#include "common/list_utils.h"
#include "common/path.h"
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

  if (mtx::sys::find_exe_in_path(mtx::fs::to_path("systemctl")).empty())
    return false;

#if defined(HAVE_QTDBUS)
  if (type == Util::Settings::RunProgramType::ShowDesktopNotification)
    return true;
#endif

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

void
LinuxProgramRunner::showDesktopNotification([[maybe_unused]] Util::Settings::RunProgramForEvent const forEvent,
                                            [[maybe_unused]] VariableMap const &variables) {
#if defined(HAVE_QTDBUS)
  QString message;

  QVariantMap hints;

  if (mtx::included_in(forEvent, Util::Settings::RunAfterJobCompletesSuccessfully, Util::Settings::RunAfterJobCompletesWithErrors, Util::Settings::RunAfterJobCompletesWithWarnings)) {
    auto description = variables.value(Q("JOB_DESCRIPTION"));
    if (description.isEmpty())
      return;

    message              = forEvent == Util::Settings::RunAfterJobCompletesSuccessfully ? QY("Job succeeded")
                         : forEvent == Util::Settings::RunAfterJobCompletesWithWarnings ? QY("Job completed with warnings")
                         :                                                                QY("Job failed");
    hints[Q("category")] = forEvent == Util::Settings::RunAfterJobCompletesSuccessfully ? Q("transfer.complete")
                         : forEvent == Util::Settings::RunAfterJobCompletesWithWarnings ? Q("transfer.complete")
                         :                                                                Q("transfer.error");

    message              = Q("%1: %2").arg(message).arg(description[0]);

  } else
    message = QY("The job queue has finished processing.");

  hints[Q("desktop-entry")] = Q("org.bunkus.mkvtoolnix-gui");
  hints[Q("urgency")]       = 0u; // low

  qDebug() << "LinuxProgramRunner::showDesktopNotification: about to notify; message & hints:" << message << hints;

  auto bus = QDBusConnection::sessionBus();

  if (!bus.isConnected()) {
    qDebug() << "LinuxProgramRunner::showDesktopNotification: error: D-Bus session bus is not connected";
    return;
  }

  QDBusInterface interface{Q("org.freedesktop.Notifications"), Q("/org/freedesktop/Notifications"), Q("org.freedesktop.Notifications"), bus};
  QDBusReply<uint32_t> const reply{
    interface.call(Q("Notify"),
                   Q("MKVToolNix GUI"),     // app_name
                   0u,                      // replaces_id
                   Q(""),                   // app_icon
                   QY("Job queue changed"), // summary
                   message,                 // body
                   QStringList{},           // actions
                   hints,                   // hints
                   -1)                      // timeout in ms; -1 = let server decide
  };

  if (!reply.isValid())
    qDebug() << "LinuxProgramRunner::showDesktopNotification: notification failed:" << reply.error();
#endif  // HAVE_QTDBUS
}

}

#endif  // !SYS_APPLE && !SYS_WINDOWS
