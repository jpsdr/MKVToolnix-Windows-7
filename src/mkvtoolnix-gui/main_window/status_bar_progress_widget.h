#pragma once

#include "common/common_pch.h"

#include <QWidget>

class QMouseEvent;

namespace mtx { namespace gui {

namespace Ui {
class StatusBarProgressWidget;
}

class StatusBarProgressWidgetPrivate;
class StatusBarProgressWidget : public QWidget {
  Q_OBJECT;

protected:
  Q_DECLARE_PRIVATE(StatusBarProgressWidget);

  QScopedPointer<StatusBarProgressWidgetPrivate> const d_ptr;

public:
  explicit StatusBarProgressWidget(QWidget *parent = nullptr);
  virtual ~StatusBarProgressWidget();

  void retranslateUi();

public slots:
  void setProgress(int progress, int totalProgress);
  void setJobStats(int numPendingAutomatic, int numPendingManual, int numRunning, int numOther);
  void setNumUnacknowledgedWarningsOrErrors(int numWarnings, int numErrors);
  void updateWarningsAndErrorsIcons();

  void reset();

protected:
  void setLabelTexts();

  virtual void mouseReleaseEvent(QMouseEvent *event) override;
};

}}
