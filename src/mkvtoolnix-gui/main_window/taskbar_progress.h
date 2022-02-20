#pragma once

#include "common/common_pch.h"

#include <QObject>

namespace mtx::gui {

namespace Jobs {
enum class QueueStatus;
class Tool;
}

class TaskbarProgressPrivate;

class TaskbarProgress : public QObject {
  Q_OBJECT

protected:
  MTX_DECLARE_PRIVATE(TaskbarProgressPrivate)

  std::unique_ptr<TaskbarProgressPrivate> const p_ptr;

public:
  explicit TaskbarProgress(QWidget *parent = nullptr);
  virtual ~TaskbarProgress();

public Q_SLOTS:
  void setup();
  void updateTaskbarProgress(int progress, int totalProgress);
  void updateTaskbarStatus(Jobs::QueueStatus status);
};

}
