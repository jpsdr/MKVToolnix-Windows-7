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

// Intentionally deactivated for the time being as there's no
// implementation for Windows yet.

// #if defined(HAVE_QTDBUS)
//   if (type == Util::Settings::RunProgramType::ShowDesktopNotification)
//     return true;
// #endif

  return mtx::included_in(type, Util::Settings::RunProgramType::ShutDownComputer, Util::Settings::RunProgramType::HibernateComputer, Util::Settings::RunProgramType::SleepComputer);
}

void
LinuxProgramRunner::systemctlAction(QString const &action) {
  qDebug() << u"LinuxProgramRunner::systemctlAction: about to execute 'systemctl %1'"_s.arg(action);

  auto args   = QStringList{} << action;
  auto result = QProcess::execute(u"systemctl"_s, args);

  if (result == 0)
    return;

  qDebug() << u"LinuxProgramRunner::systemctlAction: 'systemctl %1' failed: %2"_s.arg(action).arg(result);
}

void
LinuxProgramRunner::shutDownComputer(Util::Settings::RunProgramConfig &) {
  systemctlAction(u"poweroff"_s);
}

void
LinuxProgramRunner::hibernateComputer(Util::Settings::RunProgramConfig &) {
  systemctlAction(u"hibernate"_s);
}

void
LinuxProgramRunner::sleepComputer(Util::Settings::RunProgramConfig &) {
  systemctlAction(u"suspend"_s);
}

void
LinuxProgramRunner::showDesktopNotification([[maybe_unused]] Util::Settings::RunProgramForEvent const forEvent,
                                            [[maybe_unused]] VariableMap const &variables) {
#if defined(HAVE_QTDBUS)
  QString message;

  QVariantMap hints;

  if (mtx::included_in(forEvent, Util::Settings::RunAfterJobCompletesSuccessfully, Util::Settings::RunAfterJobCompletesWithErrors, Util::Settings::RunAfterJobCompletesWithWarnings)) {
    auto description = variables.value(u"JOB_DESCRIPTION"_s);
    if (description.isEmpty())
      return;

    message              = forEvent == Util::Settings::RunAfterJobCompletesSuccessfully ? QY("Job succeeded")
                         : forEvent == Util::Settings::RunAfterJobCompletesWithWarnings ? QY("Job completed with warnings")
                         :                                                                QY("Job failed");
    hints[u"category"_s] = forEvent == Util::Settings::RunAfterJobCompletesSuccessfully ? u"transfer.complete"_s
                         : forEvent == Util::Settings::RunAfterJobCompletesWithWarnings ? u"transfer.complete"_s
                         :                                                                u"transfer.error"_s;

    message              = u"%1: %2"_s.arg(message).arg(description[0]);

  } else
    message = QY("The job queue has finished processing.");

  hints[u"desktop-entry"_s] = u"org.bunkus.mkvtoolnix-gui"_s;
  hints[u"urgency"_s]       = 0u; // low

  qDebug() << "LinuxProgramRunner::showDesktopNotification: about to notify; message & hints:" << message << hints;

  auto bus = QDBusConnection::sessionBus();

  if (!bus.isConnected()) {
    qDebug() << "LinuxProgramRunner::showDesktopNotification: error: D-Bus session bus is not connected";
    return;
  }

  QDBusInterface interface{u"org.freedesktop.Notifications"_s, u"/org/freedesktop/Notifications"_s, u"org.freedesktop.Notifications"_s, bus};
  QDBusReply<uint32_t> const reply{
    interface.call(u"Notify"_s,
                   u"MKVToolNix GUI"_s,     // app_name
                   0u,                      // replaces_id
                   u""_s,                   // app_icon
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
