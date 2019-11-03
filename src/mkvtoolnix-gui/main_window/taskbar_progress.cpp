#include "common/common_pch.h"

#if defined(SYS_WINDOWS)

#include <QSysInfo>
#include <QWinTaskbarButton>
#include <QWinTaskbarProgress>

#include "mkvtoolnix-gui/jobs/model.h"
#include "mkvtoolnix-gui/jobs/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/main_window/taskbar_progress.h"

namespace mtx::gui {

TaskbarProgress::TaskbarProgress(QWidget *parent)
  : QObject{parent}
  , m_button{}
{
  if (QSysInfo::WV_WINDOWS7 > QSysInfo::WindowsVersion)
    return;

  m_button   = new QWinTaskbarButton{this};
  auto model = MainWindow::get()->jobTool()->model();

  connect(MainWindow::get(), &MainWindow::windowShown,         this, &TaskbarProgress::setupWindowHandle);
  connect(model,             &Jobs::Model::progressChanged,    this, &TaskbarProgress::updateTaskbarProgress);
  connect(model,             &Jobs::Model::queueStatusChanged, this, &TaskbarProgress::updateTaskbarStatus);
}

TaskbarProgress::~TaskbarProgress() {
}

void
TaskbarProgress::setupWindowHandle() {
  m_button->setWindow(static_cast<MainWindow *>(parent())->windowHandle());
}

void
TaskbarProgress::updateTaskbarProgress(int /* progress */,
                                       int totalProgress) {
  m_button->progress()->setValue(totalProgress);
}

void
TaskbarProgress::updateTaskbarStatus(Jobs::QueueStatus status) {
  auto progress  = m_button->progress();
  auto wasStopped = !progress->isVisible();
  auto nowStopped = Jobs::QueueStatus::Stopped == status;

  if (wasStopped && !nowStopped)
    progress->reset();

  progress->setVisible(!nowStopped);
}

}

#endif  // SYS_WINDOWS
