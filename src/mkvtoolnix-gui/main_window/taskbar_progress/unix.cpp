#include "common/common_pch.h"

#include <QObject>
#include <QWidget>

#include "mkvtoolnix-gui/main_window/taskbar_progress.h"

namespace mtx::gui {

class TaskbarProgressPrivate {
};

TaskbarProgress::TaskbarProgress(QWidget *parent)
  : QObject{parent}
  , p_ptr{new TaskbarProgressPrivate}
{
}

TaskbarProgress::~TaskbarProgress() {
}

void
TaskbarProgress::setup() {
}

void
TaskbarProgress::updateTaskbarProgress([[maybe_unused]] int progress,
                                       [[maybe_unused]] int totalProgress) {
}

void
TaskbarProgress::updateTaskbarStatus([[maybe_unused]] Jobs::QueueStatus status) {
}

}
