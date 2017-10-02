#pragma once

#include "common/common_pch.h"

#if defined(SYS_WINDOWS)

#include <QObject>

class QWinTaskbarButton;

namespace mtx { namespace gui {

namespace Jobs {
enum class QueueStatus;
class Tool;
}

class TaskbarProgress : public QObject {
  Q_OBJECT;

protected:
  QWinTaskbarButton *m_button;

public:
  explicit TaskbarProgress(QWidget *parent = nullptr);
  virtual ~TaskbarProgress();

public slots:
  void setupWindowHandle();
  void updateTaskbarProgress(int progress, int totalProgress);
  void updateTaskbarStatus(Jobs::QueueStatus status);
};

}}

#endif  // SYS_WINDOWS
