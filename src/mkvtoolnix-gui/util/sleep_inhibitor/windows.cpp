#include "common/common_pch.h"

#include <windows.h>

#include "mkvtoolnix-gui/util/sleep_inhibitor_p.h"
#include "mkvtoolnix-gui/util/sleep_inhibitor/windows.h"

// See https://docs.microsoft.com/en-us/windows/desktop/api/winbase/nf-winbase-setthreadexecutionstate

namespace mtx::gui::Util {

class WindowsSleepInhibitorPrivate: public BasicSleepInhibitorPrivate {
public:
  std::optional<EXECUTION_STATE> m_previousState;
};

WindowsSleepInhibitor::WindowsSleepInhibitor()
  : BasicSleepInhibitor(*new WindowsSleepInhibitorPrivate)
{
}

WindowsSleepInhibitor::~WindowsSleepInhibitor() {
}

bool
WindowsSleepInhibitor::inhibit() {
  auto p = p_func();

  mxdebug_if(p->ms_debug, fmt::format("Windows sleep inhibitor: starting\n"));

  if (p->m_previousState) {
    mxdebug_if(p->ms_debug, fmt::format("Windows sleep inhibitor: already inhibited\n"));
    return true;
  }

  auto result = SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED);

  if (result == 0) {
    mxdebug_if(p->ms_debug, fmt::format("Windows sleep inhibitor: error: thread execution state could not be set\n"));
    return false;
  }

  p->m_previousState = result;
  mxdebug_if(p->ms_debug, fmt::format("Windows sleep inhibitor: success: previous state: {0}\n", *p->m_previousState));

  return true;
}

void
WindowsSleepInhibitor::uninhibit() {
  auto p = p_func();

  mxdebug_if(p->ms_debug, fmt::format("Windows sleep inhibitor: uninhibiting: {0}\n", p->m_previousState ? fmt::format("previous state {0}", *p->m_previousState) : "nothing to do"));

  if (!p->m_previousState)
    return;

  auto result = SetThreadExecutionState(*p->m_previousState);
  p->m_previousState.reset();

  if (result == 0)
    mxdebug_if(p->ms_debug, fmt::format("Windows sleep inhibitor: error: previous state could not be restored\n"));
}

bool
WindowsSleepInhibitor::isInhibited()
  const {
  return !!p_func()->m_previousState;
}

}
