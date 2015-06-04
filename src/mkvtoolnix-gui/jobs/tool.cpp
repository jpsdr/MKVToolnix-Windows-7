#include "common/common_pch.h"

#include "common/qt.h"
#include "mkvtoolnix-gui/forms/jobs/tool.h"
#include "mkvtoolnix-gui/forms/main_window/main_window.h"
#include "mkvtoolnix-gui/jobs/mux_job.h"
#include "mkvtoolnix-gui/jobs/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/merge/mux_config.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/util.h"
#include "mkvtoolnix-gui/watch_jobs/tab.h"
#include "mkvtoolnix-gui/watch_jobs/tool.h"

#include <QList>
#include <QMenu>
#include <QMessageBox>
#include <QString>
#include <QTreeView>

namespace mtx { namespace gui { namespace Jobs {

Tool::Tool(QWidget *parent,
           QMenu *jobQueueMenu)
  : ToolBase{parent}
  , ui{new Ui::Tool}
  , m_model{new Model{this}}
  , m_startAction{new QAction{this}}
  , m_viewOutputAction{new QAction{this}}
  , m_removeAction{new QAction{this}}
  , m_acknowledgeSelectedWarningsAction{new QAction{this}}
  , m_acknowledgeSelectedErrorsAction{new QAction{this}}
  , m_jobQueueMenu{jobQueueMenu}
{
  // Setup UI controls.
  ui->setupUi(this);

  setupUiControls();
}

Tool::~Tool() {
}

void
Tool::loadAndStart() {
  auto reg = Util::Settings::registry();
  m_model->loadJobs(*reg);

  resizeColumnsToContents();

  m_model->start();
}

Model *
Tool::model()
  const {
  return m_model;
}

void
Tool::setupUiControls() {
  auto mwUi = MainWindow::getUi();

  ui->jobs->setModel(m_model);

  connect(m_jobQueueMenu,                             &QMenu::aboutToShow,       this,    &Tool::onJobQueueMenu);

  connect(mwUi->actionJobQueueRemoveDone,             &QAction::triggered,       this,    &Tool::onRemoveDone);
  connect(mwUi->actionJobQueueRemoveDoneOk,           &QAction::triggered,       this,    &Tool::onRemoveDoneOk);
  connect(mwUi->actionJobQueueRemoveAll,              &QAction::triggered,       this,    &Tool::onRemoveAll);

  connect(mwUi->actionJobQueueAcknowledgeAllWarnings, &QAction::triggered,       m_model, &Model::acknowledgeAllWarnings);
  connect(mwUi->actionJobQueueAcknowledgeAllErrors,   &QAction::triggered,       m_model, &Model::acknowledgeAllErrors);

  connect(m_startAction,                              &QAction::triggered,       this,    &Tool::onStart);
  connect(m_viewOutputAction,                         &QAction::triggered,       this,    &Tool::onViewOutput);
  connect(m_removeAction,                             &QAction::triggered,       this,    &Tool::onRemove);
  connect(m_acknowledgeSelectedWarningsAction,        &QAction::triggered,       this,    &Tool::acknowledgeSelectedWarnings);
  connect(m_acknowledgeSelectedErrorsAction,          &QAction::triggered,       this,    &Tool::acknowledgeSelectedErrors);

  connect(ui->jobs,                                   &QTreeView::doubleClicked, this,    &Tool::onViewOutput);
}

void
Tool::onJobQueueMenu() {
  auto mwUi    = MainWindow::getUi();
  auto hasJobs = m_model->hasJobs();

  mwUi->actionJobQueueRemoveDone->setEnabled(hasJobs);
  mwUi->actionJobQueueRemoveDoneOk->setEnabled(hasJobs);
  mwUi->actionJobQueueRemoveAll->setEnabled(hasJobs);

  mwUi->actionJobQueueAcknowledgeAllWarnings->setEnabled(hasJobs);
  mwUi->actionJobQueueAcknowledgeAllErrors->setEnabled(hasJobs);
}

void
Tool::onContextMenu(QPoint pos) {
  bool hasSelection = false;

  m_model->withSelectedJobs(ui->jobs, [&hasSelection](Job &) { hasSelection = true; });

  m_startAction->setEnabled(hasSelection);
  m_viewOutputAction->setEnabled(hasSelection);
  m_removeAction->setEnabled(hasSelection);

  m_acknowledgeSelectedWarningsAction->setEnabled(hasSelection);
  m_acknowledgeSelectedErrorsAction->setEnabled(hasSelection);

  QMenu menu{this};

  menu.addSeparator();
  menu.addAction(m_startAction);
  menu.addSeparator();
  menu.addAction(m_viewOutputAction);
  menu.addSeparator();
  menu.addAction(m_removeAction);
  menu.addSeparator();
  menu.addAction(m_acknowledgeSelectedWarningsAction);
  menu.addAction(m_acknowledgeSelectedErrorsAction);

  menu.exec(ui->jobs->viewport()->mapToGlobal(pos));
}

void
Tool::onStart() {
  m_model->withSelectedJobs(ui->jobs, [](Job &job) { job.setPendingAuto(); });

  m_model->startNextAutoJob();
}

void
Tool::onRemove() {
  auto idsToRemove        = QMap<uint64_t, bool>{};
  auto emitRunningWarning = false;

  m_model->withSelectedJobs(ui->jobs, [&idsToRemove](Job &job) { idsToRemove[job.m_id] = true; });

  m_model->removeJobsIf([&idsToRemove, &emitRunningWarning](Job const &job) -> bool {
    if (!idsToRemove[job.m_id])
      return false;
    if (Job::Running != job.m_status)
      return true;

    emitRunningWarning = true;
    return false;
  });

  if (emitRunningWarning)
    MainWindow::get()->setStatusBarMessage(QY("Running jobs cannot be removed."));
}

void
Tool::onRemoveDone() {
  m_model->removeJobsIf([this](Job const &job) {
      return (Job::DoneOk       == job.m_status)
          || (Job::DoneWarnings == job.m_status)
          || (Job::Failed       == job.m_status)
          || (Job::Aborted      == job.m_status);
    });
}

void
Tool::onRemoveDoneOk() {
  m_model->removeJobsIf([this](Job const &job) { return Job::DoneOk == job.m_status; });
}

void
Tool::onRemoveAll() {
  auto emitRunningWarning = false;

  m_model->removeJobsIf([&emitRunningWarning](Job const &job) -> bool {
    if (Job::Running != job.m_status)
      return true;

    emitRunningWarning = true;
    return false;
  });

  if (emitRunningWarning)
    MainWindow::get()->setStatusBarMessage(QY("Running jobs cannot be removed."));
}

void
Tool::resizeColumnsToContents()
  const {
  Util::resizeViewColumnsToContents(ui->jobs);
}

void
Tool::addJob(JobPtr const &job) {
  MainWindow::watchCurrentJobTab()->connectToJob(*job);

  m_model->add(job);
  resizeColumnsToContents();
}

void
Tool::retranslateUi() {
  ui->retranslateUi(this);
  m_model->retranslateUi();

  m_startAction->setText(QY("&Start selected jobs automatically"));
  m_viewOutputAction->setText(QY("&View output of selected jobs"));
  m_removeAction->setText(QY("&Remove selected jobs"));

  m_acknowledgeSelectedWarningsAction->setText(QY("Acknowledge selected warnings"));
  m_acknowledgeSelectedErrorsAction->setText(QY("Acknowledge selected errors"));

  setupToolTips();
}

void
Tool::setupToolTips() {
  Util::setToolTip(ui->jobs, QY("Right-click for actions for jobs"));
}

void
Tool::toolShown() {
  MainWindow::get()->showTheseMenusOnly({ m_jobQueueMenu });
}

void
Tool::acknowledgeSelectedWarnings() {
  m_model->acknowledgeSelectedWarnings(ui->jobs);
}

void
Tool::acknowledgeSelectedErrors() {
  m_model->acknowledgeSelectedErrors(ui->jobs);
}

void
Tool::onViewOutput() {
  auto tool = mtx::gui::MainWindow::watchJobTool();
  m_model->withSelectedJobs(ui->jobs, [tool](Job &job) { tool->viewOutput(job); });

  mtx::gui::MainWindow::get()->changeToTool(tool);
}

void
Tool::acknowledgeWarningsAndErrors(uint64_t id) {
  m_model->withJob(id, [](Job &job) {
    job.acknowledgeWarnings();
    job.acknowledgeErrors();
  });
}

}}}
