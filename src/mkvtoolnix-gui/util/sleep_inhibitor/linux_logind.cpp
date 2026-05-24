#include "common/common_pch.h"

#if defined(HAVE_QTDBUS)

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusUnixFileDescriptor>
#include <unistd.h>

#include "common/qt.h"
#include "mkvtoolnix-gui/util/sleep_inhibitor_p.h"
#include "mkvtoolnix-gui/util/sleep_inhibitor/linux_logind.h"

// See https://www.freedesktop.org/wiki/Software/systemd/inhibit/ for the semantics.

namespace mtx::gui::Util {

class LogindSleepInhibitorPrivate: public BasicSleepInhibitorPrivate {
public:
  std::optional<int> m_fd;
};

LogindSleepInhibitor::LogindSleepInhibitor()
  : BasicSleepInhibitor(*new LogindSleepInhibitorPrivate)
{
}

LogindSleepInhibitor::~LogindSleepInhibitor() {
}

bool
LogindSleepInhibitor::inhibit() {
  auto p = p_func();

  mxdebug_if(p->ms_debug, fmt::format("logind sleep inhibitor: starting\n"));

  if (p->m_fd) {
    mxdebug_if(p->ms_debug, fmt::format("logind sleep inhibitor: already inhibited\n"));
    return true;
  }

  auto bus = QDBusConnection::systemBus();

  if (!bus.isConnected()) {
    mxdebug_if(p->ms_debug, fmt::format("logind sleep inhibitor: error: D-Bus system bus is not connected\n"));
    return false;
  }

  QDBusInterface                            interface{Q("org.freedesktop.login1"), Q("/org/freedesktop/login1"), Q("org.freedesktop.login1.Manager"), bus};
  QDBusReply<QDBusUnixFileDescriptor> const reply    {interface.call("Inhibit", "shutdown:idle", "MKVToolNix GUI", "Jobs in progress.", "block")};

  if (!reply.isValid()) {
    mxdebug_if(p->ms_debug, fmt::format("logind sleep inhibitor: error: call failed: {0}\n", to_utf8(reply.error().message())));
    return false;
  }

  auto const fd = reply.value();
  if (!fd.isValid()) {
    mxdebug_if(p->ms_debug, fmt::format("logind sleep inhibitor: error: call returned an invalid file descriptor\n"));
    return false;
  }

  p->m_fd = fd.fileDescriptor();
  mxdebug_if(p->ms_debug, fmt::format("logind sleep inhibitor: success: file descriptor: {0}\n", *p->m_fd));

  return true;
}

void
LogindSleepInhibitor::uninhibit() {
  auto p = p_func();

  mxdebug_if(p->ms_debug, fmt::format("logind sleep inhibitor: uninhibiting: {0}\n", p->m_fd ? fmt::format("file descriptor {0}", *p->m_fd) : "nothing to do"));

  if (!p->m_fd)
    return;

  ::close(*p->m_fd);
  p->m_fd.reset();
}

bool
LogindSleepInhibitor::isInhibited()
  const {
  return !!p_func()->m_fd;
}

}

#endif  // defined(HAVE_QTDBUS)
