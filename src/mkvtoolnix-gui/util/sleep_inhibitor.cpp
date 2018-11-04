#include "common/common_pch.h"

#include "mkvtoolnix-gui/util/sleep_inhibitor.h"
#include "mkvtoolnix-gui/util/sleep_inhibitor_p.h"
#if !defined(SYS_WINDOWS) && !defined(SYS_APPLE)
#include "mkvtoolnix-gui/util/sleep_inhibitor/linux_logind.h"
#endif

namespace mtx { namespace gui { namespace Util {

debugging_option_c BasicSleepInhibitorPrivate::ms_debug{"sleep_inhibitor"};

BasicSleepInhibitor::BasicSleepInhibitor()
  : p_ptr{new BasicSleepInhibitorPrivate}
{
}

BasicSleepInhibitor::BasicSleepInhibitor(BasicSleepInhibitorPrivate &p)
  : p_ptr{&p}
{
}

BasicSleepInhibitor::~BasicSleepInhibitor() {
  uninhibit();
}

bool
BasicSleepInhibitor::inhibit() {
  for (auto const &inhibitor : p_func()->m_inhibitors)
    if (inhibitor->inhibit())
      return true;

  return false;
}

void
BasicSleepInhibitor::uninhibit() {
  for (auto const &inhibitor : p_func()->m_inhibitors)
    inhibitor->uninhibit();
}

bool
BasicSleepInhibitor::isInhibited()
  const {
  for (auto const &inhibitor : p_func()->m_inhibitors)
    if (inhibitor->isInhibited())
      return true;

  return false;
}

void
BasicSleepInhibitor::addInhibitor(std::shared_ptr<BasicSleepInhibitor> const &inhibitor) {
  p_func()->m_inhibitors.emplace_back(inhibitor);
}

std::unique_ptr<BasicSleepInhibitor>
BasicSleepInhibitor::create() {
  auto inhibitor = std::make_unique<BasicSleepInhibitor>();

#if !defined(SYS_WINDOWS) && !defined(SYS_APPLE)
  inhibitor->addInhibitor(std::make_shared<LogindSleepInhibitor>());
#endif

 return inhibitor;
}

}}}
