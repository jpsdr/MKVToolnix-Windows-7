#pragma once

#include "common/common_pch.h"

#include <QWidget>

#include "common/qt.h"

class QMouseEvent;

namespace mtx::gui {

namespace Ui {
class StatusBarProgressWidget;
}

class StatusBarProgressWidgetPrivate;
class StatusBarProgressWidget : public QWidget {
  Q_OBJECT

protected:
  MTX_DECLARE_PRIVATE(StatusBarProgressWidgetPrivate)

  std::unique_ptr<StatusBarProgressWidgetPrivate> const p_ptr;

  explicit StatusBarProgressWidget(StatusBarProgressWidgetPrivate &p);

public:
  explicit StatusBarProgressWidget(QWidget *parent = nullptr);
  virtual ~StatusBarProgressWidget();

  void retranslateUi();

public Q_SLOTS:
  void setProgress(int progress, int totalProgress);
  void setJobStats(int numPendingAutomatic, int numPendingManual, int numRunning, int numOther);
  void setNumUnacknowledgedWarningsOrErrors(int numOldWarnings, int numCurrentWarnings, int numOldErrors, int numCurrentErrors);
  void updateWarningsAndErrorsIcons();

  void reset();

protected:
  void setLabelTextsAndToolTip();

  virtual void mouseReleaseEvent(QMouseEvent *event) override;
};

}
