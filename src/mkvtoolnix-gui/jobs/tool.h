#ifndef MTX_MKVTOOLNIX_GUI_JOBS_JOBS_WIDGET_H
#define MTX_MKVTOOLNIX_GUI_JOBS_JOBS_WIDGET_H

#include "common/common_pch.h"

#include <QList>

#include "mkvtoolnix-gui/main_window/tool_base.h"
#include "mkvtoolnix-gui/jobs/model.h"

class QAction;
class QMenu;

namespace mtx { namespace gui { namespace Jobs {

namespace Ui {
class Tool;
}

class Tool : public ToolBase {
  Q_OBJECT;

protected:
  // UI stuff:
  std::unique_ptr<Ui::Tool> ui;

  Model *m_model;

  QAction *m_startAutomaticallyAction, *m_startManuallyAction, *m_viewOutputAction, *m_removeAction, *m_acknowledgeSelectedWarningsAction, *m_acknowledgeSelectedErrorsAction, *m_acknowledgeSelectedWarningsErrorsAction;
  QAction *m_openFolderAction, *m_editAndRemoveAction, *m_startImmediatelyAction;
  QMenu *m_jobQueueMenu, *m_jobsMenu;

public:
  explicit Tool(QWidget *parent, QMenu *jobQueueMenu);
  ~Tool();

  virtual void setupUi() override;
  virtual void setupActions() override;

  Model *model() const;
  void addJob(JobPtr const &job);
  bool addJobFile(QString const &fileName);
  void loadAndStart();

public slots:
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

  void onJobQueueMenu();
  void onContextMenu(QPoint pos);

  void resizeColumnsToContents() const;

  virtual void toolShown() override;

  void acknowledgeSelectedWarnings();
  void acknowledgeSelectedErrors();
  void acknowledgeWarningsAndErrors(uint64_t id);

protected:
  void setupToolTips();

  void stopQueue(bool immediately);

  void openJobInTool(Job const &job) const;
};

}}}

#endif // MTX_MKVTOOLNIX_GUI_JOBS_JOB_WIDGET_H
