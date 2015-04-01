#ifndef MTX_MKVTOOLNIX_GUI_WATCH_JOB_CONTAINER_WIDGET_WATCH_JOB_CONTAINER_WIDGET_H
#define MTX_MKVTOOLNIX_GUI_WATCH_JOB_CONTAINER_WIDGET_WATCH_JOB_CONTAINER_WIDGET_H

#include "common/common_pch.h"

#include "mkvtoolnix-gui/main_window/tool_base.h"

namespace Ui {
class WatchJobContainerWidget;
}

class WatchJobWidget;

class WatchJobContainerWidget : public ToolBase {
  Q_OBJECT;

protected:
  // UI stuff:
  std::unique_ptr<Ui::WatchJobContainerWidget> ui;
  WatchJobWidget *m_currentJobWidget;

public:
  explicit WatchJobContainerWidget(QWidget *parent = nullptr);
  ~WatchJobContainerWidget();

  WatchJobWidget *currentJobWidget();

  virtual void retranslateUi() override;

public slots:
  virtual void toolShown() override;
};

#endif // MTX_MKVTOOLNIX_GUI_WATCH_JOB_CONTAINER_WIDGET_WATCH_JOB_CONTAINER_WIDGET_H
