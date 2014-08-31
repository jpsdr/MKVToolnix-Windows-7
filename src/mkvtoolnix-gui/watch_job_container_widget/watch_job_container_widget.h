#ifndef MTX_MKVTOOLNIX_GUI_WATCH_JOB_CONTAINER_WIDGET_WATCH_JOB_CONTAINER_WIDGET_H
#define MTX_MKVTOOLNIX_GUI_WATCH_JOB_CONTAINER_WIDGET_WATCH_JOB_CONTAINER_WIDGET_H

#include "common/common_pch.h"

#include <QWidget>

namespace Ui {
class WatchJobContainerWidget;
}

class WatchJobWidget;

class WatchJobContainerWidget : public QWidget {
  Q_OBJECT;

protected:
  // UI stuff:
  std::unique_ptr<Ui::WatchJobContainerWidget> ui;
  WatchJobWidget *m_currentJobWidget;

public:
  explicit WatchJobContainerWidget(QWidget *parent = nullptr);
  ~WatchJobContainerWidget();

  WatchJobWidget *currentJobWidget();

public slots:

protected:
  virtual void retranslateUi();
};

#endif // MTX_MKVTOOLNIX_GUI_WATCH_JOB_CONTAINER_WIDGET_WATCH_JOB_CONTAINER_WIDGET_H
