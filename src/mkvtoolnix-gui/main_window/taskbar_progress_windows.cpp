#include "common/common_pch.h"

#include <QWinTaskbarButton>
#include <QWinTaskbarProgress>

#include "mkvtoolnix-gui/jobs/model.h"
#include "mkvtoolnix-gui/jobs/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/main_window/taskbar_progress.h"

namespace mtx::gui {

class TaskbarProgressPrivate {
  friend class TaskbarProgress;

  QWinTaskbarButton *m_button{};

  explicit TaskbarProgressPrivate(QWidget *parent)
    : m_button{new QWinTaskbarButton{parent}}
  {
  }
};

TaskbarProgress::TaskbarProgress(QWidget *parent)
  : QObject{parent}
  , p_ptr{new TaskbarProgressPrivate{parent}}
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
  p_func()->m_button->setWindow(static_cast<MainWindow *>(parent())->windowHandle());
}

void
TaskbarProgress::updateTaskbarProgress([[maybe_unused]] int progress,
                                       int totalProgress) {
  p_func()->m_button->progress()->setValue(totalProgress);
}

void
TaskbarProgress::updateTaskbarStatus(Jobs::QueueStatus status) {
  auto &p         = *p_func();
  auto progress   = p.m_button->progress();
  auto wasStopped = !progress->isVisible();
  auto nowStopped = Jobs::QueueStatus::Stopped == status;

  if (wasStopped && !nowStopped)
    progress->reset();

  progress->setVisible(!nowStopped);
}

}
