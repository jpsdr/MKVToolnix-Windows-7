#ifndef MTX_MKVTOOLNIX_GUI_MAIN_WINDOW_STATUS_BAR_PROGRESS_WIDGET_H
#define MTX_MKVTOOLNIX_GUI_MAIN_WINDOW_STATUS_BAR_PROGRESS_WIDGET_H

#include "common/common_pch.h"

#include <QWidget>

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

  void showContextMenu(QPoint const &pos);

  void reset();

protected:
  void setLabelTexts();
};

}}

#endif // MTX_MKVTOOLNIX_GUI_MAIN_WINDOW_STATUS_BAR_PROGRESS_WIDGET_H
