#pragma once

#include "common/common_pch.h"

#include <QWidget>

#include "mkvtoolnix-gui/jobs/job.h"

class QDateTime;
class QLabel;

namespace mtx { namespace gui { namespace WatchJobs {

namespace Ui {
class Tab;
}

class TabPrivate;

class Tab : public QWidget {
  Q_OBJECT;

private:
  Q_DISABLE_COPY(Tab);
  Q_DECLARE_PRIVATE(Tab);

  QScopedPointer<TabPrivate> const d_ptr;

public:
  explicit Tab(QWidget *parent, bool forCurrentJob = false);
  ~Tab();

  virtual void retranslateUi();

  virtual void connectToJob(mtx::gui::Jobs::Job const &job);
  virtual void disconnectFromJob(mtx::gui::Jobs::Job const &job);
  virtual void setInitialDisplay(mtx::gui::Jobs::Job const &job);

  virtual uint64_t queueProgress() const;

  uint64_t id() const;

  bool isSaveOutputEnabled() const;
  bool isCurrentJobTab() const;

signals:
  void abortJob();
  void watchCurrentJobTabCleared();

public slots:
  void onStatusChanged(uint64_t id, mtx::gui::Jobs::Job::Status oldStatus, mtx::gui::Jobs::Job::Status newStatus);
  void onJobProgressChanged(uint64_t id, unsigned int progress);
  void onQueueProgressChanged(int progress, int totalProgress);
  void onLineRead(QString const &line, mtx::gui::Jobs::Job::LineType type);
  void onAbort();

  void onSaveOutput();
  void clearOutput();
  void openFolder();

  void acknowledgeWarningsAndErrors();
  void disableButtonIfAllWarningsAndErrorsButtonAcknowledged(int numWarnings, int numErrors);

  void updateRemainingTime();

  void enableMoreActionsActions();
  void setupWhenFinishedActions();

  void toggleActionToExecute();

protected:
  void setupUi();
  void setupMoreActionsMenu();

  static void updateOneRemainingTimeLabel(QLabel *label, QDateTime const &startTime, uint64_t progress);
};

}}}
