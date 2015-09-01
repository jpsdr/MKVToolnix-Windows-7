#include "common/common_pch.h"

#include "common/qt.h"
#include "mkvtoolnix-gui/forms/jobs/tool.h"
#include "mkvtoolnix-gui/forms/main_window/main_window.h"
#include "mkvtoolnix-gui/jobs/mux_job.h"
#include "mkvtoolnix-gui/jobs/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/merge/mux_config.h"
#include "mkvtoolnix-gui/merge/tool.h"
#include "mkvtoolnix-gui/util/header_view_manager.h"
#include "mkvtoolnix-gui/util/message_box.h"
#include "mkvtoolnix-gui/util/model.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/widget.h"
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
  , m_startAutomaticallyAction{new QAction{this}}
  , m_startManuallyAction{new QAction{this}}
  , m_viewOutputAction{new QAction{this}}
  , m_removeAction{new QAction{this}}
  , m_acknowledgeSelectedWarningsAction{new QAction{this}}
  , m_acknowledgeSelectedErrorsAction{new QAction{this}}
  , m_acknowledgeSelectedWarningsErrorsAction{new QAction{this}}
  , m_openFolderAction{new QAction{this}}
  , m_editAndRemoveAction{new QAction{this}}
  , m_startImmediatelyAction{new QAction{this}}
  , m_jobQueueMenu{jobQueueMenu}
  , m_jobsMenu{new QMenu{this}}
{
  // Setup UI controls.
  ui->setupUi(this);
}

Tool::~Tool() {
}

void
Tool::loadAndStart() {
  Model::convertJobQueueToSeparateIniFiles();
  m_model->loadJobs();

  resizeColumnsToContents();

  m_model->start();
}

Model *
Tool::model()
  const {
  return m_model;
}

void
Tool::setupUi() {
  ui->jobs->setModel(m_model);

  Util::preventScrollingWithoutFocus(this);
  Util::HeaderViewManager::create(*ui->jobs, "Jobs::Jobs");

  m_jobsMenu->addAction(m_viewOutputAction);
  m_jobsMenu->addAction(m_openFolderAction);
  m_jobsMenu->addSeparator();
  m_jobsMenu->addAction(m_startAutomaticallyAction);
  m_jobsMenu->addAction(m_startManuallyAction);
  m_jobsMenu->addAction(m_startImmediatelyAction);
  m_jobsMenu->addSeparator();
  m_jobsMenu->addAction(m_editAndRemoveAction);
  m_jobsMenu->addSeparator();
  m_jobsMenu->addAction(m_removeAction);
  m_jobsMenu->addSeparator();
  m_jobsMenu->addAction(m_acknowledgeSelectedWarningsAction);
  m_jobsMenu->addAction(m_acknowledgeSelectedErrorsAction);
  m_jobsMenu->addAction(m_acknowledgeSelectedWarningsErrorsAction);

  retranslateUi();
}

void
Tool::setupActions() {
  auto mw   = MainWindow::get();
  auto mwUi = MainWindow::getUi();

  connect(m_jobQueueMenu,                                   &QMenu::aboutToShow,                        this,    &Tool::onJobQueueMenu);

  connect(mwUi->actionJobQueueStartAllPending,              &QAction::triggered,                        this,    &Tool::onStartAllPending);

  connect(mwUi->actionJobQueueStopAfterCurrentJob,          &QAction::triggered,                        this,    &Tool::onStopQueueAfterCurrentJob);
  connect(mwUi->actionJobQueueStopImmediately,              &QAction::triggered,                        this,    &Tool::onStopQueueImmediately);

  connect(mwUi->actionJobQueueRemoveDone,                   &QAction::triggered,                        this,    &Tool::onRemoveDone);
  connect(mwUi->actionJobQueueRemoveDoneOk,                 &QAction::triggered,                        this,    &Tool::onRemoveDoneOk);
  connect(mwUi->actionJobQueueRemoveAll,                    &QAction::triggered,                        this,    &Tool::onRemoveAll);

  connect(mwUi->actionJobQueueAcknowledgeAllWarnings,       &QAction::triggered,                        m_model, &Model::acknowledgeAllWarnings);
  connect(mwUi->actionJobQueueAcknowledgeAllErrors,         &QAction::triggered,                        m_model, &Model::acknowledgeAllErrors);
  connect(mwUi->actionJobQueueAcknowledgeAllWarningsErrors, &QAction::triggered,                        m_model, &Model::acknowledgeAllWarnings);
  connect(mwUi->actionJobQueueAcknowledgeAllWarningsErrors, &QAction::triggered,                        m_model, &Model::acknowledgeAllErrors);

  connect(m_startAutomaticallyAction,                       &QAction::triggered,                        this,    &Tool::onStartAutomatically);
  connect(m_startManuallyAction,                            &QAction::triggered,                        this,    &Tool::onStartManually);
  connect(m_viewOutputAction,                               &QAction::triggered,                        this,    &Tool::onViewOutput);
  connect(m_removeAction,                                   &QAction::triggered,                        this,    &Tool::onRemove);
  connect(m_acknowledgeSelectedWarningsAction,              &QAction::triggered,                        this,    &Tool::acknowledgeSelectedWarnings);
  connect(m_acknowledgeSelectedErrorsAction,                &QAction::triggered,                        this,    &Tool::acknowledgeSelectedErrors);
  connect(m_acknowledgeSelectedWarningsErrorsAction,        &QAction::triggered,                        this,    &Tool::acknowledgeSelectedWarnings);
  connect(m_acknowledgeSelectedWarningsErrorsAction,        &QAction::triggered,                        this,    &Tool::acknowledgeSelectedErrors);
  connect(m_openFolderAction,                               &QAction::triggered,                        this,    &Tool::onOpenFolder);
  connect(m_editAndRemoveAction,                            &QAction::triggered,                        this,    &Tool::onEditAndRemove);
  connect(m_startImmediatelyAction,                         &QAction::triggered,                        this,    &Tool::onStartImmediately);

  connect(ui->jobs,                                         &QTreeView::doubleClicked,                  this,    &Tool::onViewOutput);

  connect(mw,                                               &MainWindow::preferencesChanged,            this,    &Tool::retranslateUi);
  connect(mw,                                               &MainWindow::aboutToClose,                  m_model, &Model::saveJobs);

  connect(MainWindow::watchCurrentJobTab(),                 &WatchJobs::Tab::watchCurrentJobTabCleared, m_model, &Model::resetTotalProgress);
}

void
Tool::onJobQueueMenu() {
  auto mwUi             = MainWindow::getUi();
  auto hasJobs          = m_model->hasJobs();
  auto hasPendingManual = false;

  m_model->withAllJobs([&hasPendingManual](Job &job) {
    if (Job::PendingManual == job.status())
      hasPendingManual = true;
  });

  mwUi->actionJobQueueStartAllPending->setEnabled(hasPendingManual);

  mwUi->actionJobQueueRemoveDone->setEnabled(hasJobs);
  mwUi->actionJobQueueRemoveDoneOk->setEnabled(hasJobs);
  mwUi->actionJobQueueRemoveAll->setEnabled(hasJobs);

  mwUi->actionJobQueueAcknowledgeAllWarnings->setEnabled(hasJobs);
  mwUi->actionJobQueueAcknowledgeAllErrors->setEnabled(hasJobs);
  mwUi->actionJobQueueAcknowledgeAllWarningsErrors->setEnabled(hasJobs);
}

void
Tool::onContextMenu(QPoint pos) {
  auto hasSelection = false, hasNotRunning = false;

  m_model->withSelectedJobs(ui->jobs, [&hasSelection, &hasNotRunning](Job const &job) {
    hasSelection = true;
    if (Job::Running != job.status())
      hasNotRunning = true;
  });

  m_startAutomaticallyAction->setEnabled(hasNotRunning);
  m_startManuallyAction->setEnabled(hasNotRunning);
  m_viewOutputAction->setEnabled(hasSelection);
  m_removeAction->setEnabled(hasSelection);
  m_openFolderAction->setEnabled(hasSelection);
  m_editAndRemoveAction->setEnabled(hasSelection);
  m_startImmediatelyAction->setEnabled(hasNotRunning);

  m_acknowledgeSelectedWarningsAction->setEnabled(hasSelection);
  m_acknowledgeSelectedErrorsAction->setEnabled(hasSelection);
  m_acknowledgeSelectedWarningsErrorsAction->setEnabled(hasSelection);

  m_jobsMenu->exec(ui->jobs->viewport()->mapToGlobal(pos));
}

void
Tool::onStartAutomatically() {
  m_model->withSelectedJobs(ui->jobs, [](Job &job) { job.setPendingAuto(); });

  m_model->startNextAutoJob();

  if (Util::Settings::get().m_switchToJobOutputAfterStarting)
    MainWindow::get()->switchToTool(MainWindow::watchJobTool());
}

void
Tool::onStartManually() {
  m_model->withSelectedJobs(ui->jobs, [](Job &job) { job.setPendingManual(); });
}

void
Tool::onStartAllPending() {
  auto startedSomething = false;

  m_model->withAllJobs([&startedSomething](Job &job) {
    if (Job::PendingManual == job.status()) {
      job.setPendingAuto();
      startedSomething = true;
    }
  });

  m_model->startNextAutoJob();

  if (startedSomething && Util::Settings::get().m_switchToJobOutputAfterStarting)
    MainWindow::get()->switchToTool(MainWindow::watchJobTool());
}

void
Tool::onStartImmediately() {
  m_model->withSelectedJobs(ui->jobs, [this](Job &job) { m_model->startJobImmediately(job); });

  if (Util::Settings::get().m_switchToJobOutputAfterStarting)
    MainWindow::get()->switchToTool(MainWindow::watchJobTool());
}

void
Tool::onRemove() {
  auto idsToRemove        = QMap<uint64_t, bool>{};
  auto emitRunningWarning = false;

  m_model->withSelectedJobs(ui->jobs, [&idsToRemove](Job &job) { idsToRemove[job.id()] = true; });

  m_model->removeJobsIf([&idsToRemove, &emitRunningWarning](Job const &job) -> bool {
    if (!idsToRemove[job.id()])
      return false;
    if (Job::Running != job.status())
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
      return (Job::DoneOk       == job.status())
          || (Job::DoneWarnings == job.status())
          || (Job::Failed       == job.status())
          || (Job::Aborted      == job.status());
    });
}

void
Tool::onRemoveDoneOk() {
  m_model->removeJobsIf([this](Job const &job) { return Job::DoneOk == job.status(); });
}

void
Tool::onRemoveAll() {
  auto emitRunningWarning = false;

  m_model->removeJobsIf([&emitRunningWarning](Job const &job) -> bool {
    if (Job::Running != job.status())
      return true;

    emitRunningWarning = true;
    return false;
  });

  if (emitRunningWarning)
    MainWindow::get()->setStatusBarMessage(QY("Running jobs cannot be removed."));
}

void
Tool::onStopQueueAfterCurrentJob() {
  stopQueue(false);
}

void
Tool::onStopQueueImmediately() {
  stopQueue(true);
}

void
Tool::resizeColumnsToContents()
  const {
  Util::resizeViewColumnsToContents(ui->jobs);
}

void
Tool::addJob(JobPtr const &job) {
  job->saveQueueFile();
  m_model->add(job);
  resizeColumnsToContents();
}

void
Tool::retranslateUi() {
  ui->retranslateUi(this);
  m_model->retranslateUi();

  m_viewOutputAction->setText(QY("&View output"));
  m_startAutomaticallyAction->setText(QY("&Start jobs automatically"));
  m_startManuallyAction->setText(QY("Start jobs &manually"));
  m_startImmediatelyAction->setText(QY("Start jobs &immediately"));
  m_removeAction->setText(QY("&Remove jobs"));
  m_openFolderAction->setText(QY("&Open folder"));
  m_editAndRemoveAction->setText(QY("&Edit in corresponding tool and remove from queue"));

  m_acknowledgeSelectedWarningsAction->setText(QY("Acknowledge warnings"));
  m_acknowledgeSelectedErrorsAction->setText(QY("Acknowledge errors"));
  m_acknowledgeSelectedWarningsErrorsAction->setText(QY("Acknowledge warnings and errors"));

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

  mtx::gui::MainWindow::get()->switchToTool(tool);
}

void
Tool::onOpenFolder() {
  m_model->withSelectedJobs(ui->jobs, [](Job const &job) { job.openOutputFolder(); });
}

void
Tool::acknowledgeWarningsAndErrors(uint64_t id) {
  m_model->withJob(id, [](Job &job) {
    job.acknowledgeWarnings();
    job.acknowledgeErrors();
  });
}

void
Tool::openJobInTool(Job const &job)
  const {
  if (dynamic_cast<MuxJob const *>(&job)) {
    auto tool = mtx::gui::MainWindow::mergeTool();
    mtx::gui::MainWindow::get()->switchToTool(tool);

    tool->openFromConfig(static_cast<MuxJob const &>(job).config());

    return;
  }

  Q_ASSERT(false);
}

void
Tool::onEditAndRemove() {
  auto emitRunningWarning = false;

  m_model->withSelectedJobs(ui->jobs, [this, &emitRunningWarning](Job &jobToEdit) {
    if (Job::Running == jobToEdit.status())
      emitRunningWarning = true;

    else {
      openJobInTool(jobToEdit);
      m_model->removeJobsIf([&jobToEdit](Job const &jobToRemove) { return jobToEdit.id() == jobToRemove.id(); });
    }
  });

  if (emitRunningWarning)
    MainWindow::get()->setStatusBarMessage(QY("Running jobs cannot be edited."));
}

void
Tool::stopQueue(bool immediately) {
  auto askBeforeAborting   = Util::Settings::get().m_warnBeforeAbortingJobs;
  auto askedBeforeAborting = false;

  m_model->withAllJobs([](Job &job) {
    job.action([&job]() {
      if (job.status() == Job::PendingAuto)
        job.setPendingManual();
    });
  });

  if (!immediately)
    return;

  m_model->withAllJobs([this, askBeforeAborting, &askedBeforeAborting](Job &job) {
    job.action([this, &job, askBeforeAborting, &askedBeforeAborting]() {
      if (job.status() != Job::Running)
        return;

      if (   !askBeforeAborting
          ||  askedBeforeAborting
          || (Util::MessageBox::question(this)
              ->title(QY("Abort running job"))
              .text(QY("Do you really want to abort the currently running job?"))
              .buttonLabel(QMessageBox::Yes, QY("&Abort job"))
              .buttonLabel(QMessageBox::No,  QY("Cancel"))
              .exec() == QMessageBox::Yes))
        job.abort();

      askedBeforeAborting = true;
    });
  });
}

}}}
