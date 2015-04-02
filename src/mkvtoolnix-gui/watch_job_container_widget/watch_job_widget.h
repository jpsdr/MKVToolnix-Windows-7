#ifndef MTX_MKVTOOLNIX_GUI_WATCH_JOB_CONTAINER_WIDGET_WATCH_JOB_WIDGET_H
#define MTX_MKVTOOLNIX_GUI_WATCH_JOB_CONTAINER_WIDGET_WATCH_JOB_WIDGET_H

#include "common/common_pch.h"

#include <QWidget>

#include "mkvtoolnix-gui/jobs/job.h"

namespace Ui {
class WatchJobWidget;
}

class WatchJobWidget : public QWidget {
  Q_OBJECT;

protected:
  // UI stuff:
  std::unique_ptr<Ui::WatchJobWidget> ui;

public:
  explicit WatchJobWidget(QWidget *parent = nullptr);
  ~WatchJobWidget();

  virtual void connectToJob(mtx::gui::Jobs::Job const &job);
  virtual void setInitialDisplay(mtx::gui::Jobs::Job const &job);

public slots:
  void onStatusChanged(uint64_t id, mtx::gui::Jobs::Job::Status status);
  void onProgressChanged(uint64_t id, unsigned int progress);
  void onLineRead(QString const &line, mtx::gui::Jobs::Job::LineType type);

protected:
};

#endif // MTX_MKVTOOLNIX_GUI_WATCH_JOB_CONTAINER_WIDGET_WATCH_JOB_WIDGET_H
