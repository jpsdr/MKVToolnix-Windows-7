#include "common/common_pch.h"

#include <QWindow>

#include <dwmapi.h>
#include <shobjidl.h>

#include "mkvtoolnix-gui/jobs/model.h"
#include "mkvtoolnix-gui/jobs/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/main_window/taskbar_progress.h"

namespace mtx::gui {

class TaskbarProgressPrivate {
  friend class TaskbarProgress;

  ITaskbarList4 *m_taskbarList{};
  HWND m_handle{};
  bool m_running{};

  explicit TaskbarProgressPrivate();

public:
  virtual ~TaskbarProgressPrivate();
};

TaskbarProgressPrivate::TaskbarProgressPrivate()
{
  auto hresult = CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskbarList4, reinterpret_cast<void **>(&m_taskbarList));

  if (FAILED(hresult)) {
    m_taskbarList = nullptr;
    return;
  }

  if (FAILED(m_taskbarList->HrInit())) {
    m_taskbarList->Release();
    m_taskbarList = nullptr;
  }
}

TaskbarProgressPrivate::~TaskbarProgressPrivate() {
  if (m_taskbarList)
    m_taskbarList->Release();
}

// ------------------------------------------------------------

TaskbarProgress::TaskbarProgress(QWidget *parent)
  : QObject{parent}
  , p_ptr{new TaskbarProgressPrivate}
{
  auto model = MainWindow::get()->jobTool()->model();

  connect(MainWindow::get(), &MainWindow::windowShown,         this, &TaskbarProgress::setup);
  connect(model,             &Jobs::Model::progressChanged,    this, &TaskbarProgress::updateTaskbarProgress);
  connect(model,             &Jobs::Model::queueStatusChanged, this, &TaskbarProgress::updateTaskbarStatus);
}

TaskbarProgress::~TaskbarProgress() {
}

void
TaskbarProgress::setup() {
  p_func()->m_handle = reinterpret_cast<HWND>(static_cast<MainWindow *>(parent())->windowHandle()->winId());
}

void
TaskbarProgress::updateTaskbarProgress([[maybe_unused]] int progress,
                                       int totalProgress) {
  auto &p = *p_func();

  if (p.m_taskbarList)
    p.m_taskbarList->SetProgressValue(p.m_handle, ULONGLONG(totalProgress), 100);
}

void
TaskbarProgress::updateTaskbarStatus(Jobs::QueueStatus status) {
  auto &p         = *p_func();
  auto newRunning = Jobs::QueueStatus::Stopped != status;

  if (!p.m_taskbarList || (p.m_running == newRunning))
    return;

  p.m_running   = newRunning;
  auto newState = newRunning ? TBPF_NORMAL : TBPF_NOPROGRESS;

  p.m_taskbarList->SetProgressState(p.m_handle, newState);
}

}
