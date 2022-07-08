#pragma once

#include "common/common_pch.h"

#include <QList>

#include "mkvtoolnix-gui/main_window/tool_base.h"
#include "mkvtoolnix-gui/jobs/model.h"
#include "mkvtoolnix-gui/util/files_drag_drop_handler.h"

class QAction;
class QMenu;

namespace mtx::gui::Jobs {

namespace Ui {
class Tool;
}

class Tool : public ToolBase {
  Q_OBJECT

protected:
  // UI stuff:
  std::unique_ptr<Ui::Tool> ui;

  Model *m_model;

  QAction *m_startAutomaticallyAction, *m_startManuallyAction, *m_viewOutputAction, *m_removeAction, *m_acknowledgeSelectedWarningsAction, *m_acknowledgeSelectedErrorsAction, *m_acknowledgeSelectedWarningsErrorsAction;
  QAction *m_openFolderAction, *m_editAndRemoveAction, *m_startImmediatelyAction;
  QMenu *m_jobQueueMenu, *m_jobsMenu;

  mtx::gui::Util::FilesDragDropHandler m_filesDDHandler;
  QStringList m_droppedFiles;

public:
  explicit Tool(QWidget *parent, QMenu *jobQueueMenu);
  ~Tool();

  virtual void setupUi() override;
  virtual void setupActions() override;

  Model *model() const;
  void addJob(JobPtr const &job);
  bool addJobFile(QString const &fileName);
  bool addDroppedFileAsJob(QString const &fileName);
  void loadAndStart();

  bool checkIfOverwritingExistingFileIsOK(QString const &existingDestination);
  bool checkIfOverwritingExistingJobIsOK(QString const &newDestination, bool isMuxJobAndSplittingEnabled = false);

public Q_SLOTS:
  virtual void retranslateUi();
  void onStartAutomatically();
  void onStartManually();
  void onStartImmediately();
  void onStartAllPending();
  void onStopQueueAfterCurrentJob();
  void onStopQueueImmediately();
  void onViewOutput();
  void onRemove();
  void onRemoveDone();
  void onRemoveDoneOk();
  void onRemoveAll();
  void onOpenFolder();
  void onEditAndRemove();
  void sortJobs(int logicalColumnIndex, Qt::SortOrder order);
  void hideSortIndicator();

  void onJobQueueMenu();
  void onContextMenu(QPoint pos);
  void moveJobsUpOrDown(bool up);
  void setupMoveJobsButtons();
  void enableMoveJobsButtons();

  virtual void toolShown() override;

  void acknowledgeSelectedWarnings();
  void acknowledgeSelectedErrors();
  void acknowledgeWarningsAndErrors(uint64_t id);

  virtual void processDroppedFiles();

protected:
  void setupToolTips();

  void stopQueue(bool immediately);

  void openJobInTool(Job const &job) const;

  virtual void dragEnterEvent(QDragEnterEvent *event) override;
  virtual void dropEvent(QDropEvent *event) override;

  virtual void selectJobs(QList<Job *> const &jobs);
};

}
