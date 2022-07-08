#include "common/common_pch.h"

#include "common/list_utils.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/app.h"
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

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QList>
#include <QMenu>
#include <QMessageBox>
#include <QString>
#include <QTreeView>

namespace mtx::gui::Jobs {

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
  , m_filesDDHandler{Util::FilesDragDropHandler::Mode::Remember}
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

  setupMoveJobsButtons();

  Util::preventScrollingWithoutFocus(this);
  Util::HeaderViewManager::create(*ui->jobs, "Jobs::Jobs").setDefaultSizes({ { Q("status"), 150 }, { Q("description"), 300 }, { Q("dateAdded"), 130 }, { Q("dateStarted"), 130 }, { Q("dateFinished"), 130 } });

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

  m_viewOutputAction->setIcon(QIcon::fromTheme(Q("layer-visible-on")));
  m_openFolderAction->setIcon(QIcon::fromTheme(Q("document-open-folder")));
  m_startAutomaticallyAction->setIcon(QIcon::fromTheme(Q("media-playback-start")));
  m_startImmediatelyAction->setIcon(QIcon::fromTheme(Q("media-seek-forward")));
  m_editAndRemoveAction->setIcon(QIcon::fromTheme(Q("edit-entry")));
  m_removeAction->setIcon(QIcon::fromTheme(Q("list-remove")));
  m_acknowledgeSelectedWarningsErrorsAction->setIcon(QIcon::fromTheme(Q("dialog-ok-apply")));

  ui->jobs->header()->setSectionsClickable(true);

  retranslateUi();
}

void
Tool::setupActions() {
  auto mw   = MainWindow::get();
  auto mwUi = MainWindow::getUi();

  connect(m_jobQueueMenu,                                   &QMenu::aboutToShow,                              this,    &Tool::onJobQueueMenu);

  connect(mwUi->actionJobQueueStartAllPending,              &QAction::triggered,                              this,    &Tool::onStartAllPending);

  connect(mwUi->actionJobQueueStopAfterCurrentJob,          &QAction::triggered,                              this,    &Tool::onStopQueueAfterCurrentJob);
  connect(mwUi->actionJobQueueStopImmediately,              &QAction::triggered,                              this,    &Tool::onStopQueueImmediately);

  connect(mwUi->actionJobQueueRemoveDone,                   &QAction::triggered,                              this,    &Tool::onRemoveDone);
  connect(mwUi->actionJobQueueRemoveDoneOk,                 &QAction::triggered,                              this,    &Tool::onRemoveDoneOk);
  connect(mwUi->actionJobQueueRemoveAll,                    &QAction::triggered,                              this,    &Tool::onRemoveAll);

  connect(mwUi->actionJobQueueAcknowledgeAllWarnings,       &QAction::triggered,                              m_model, &Model::acknowledgeAllWarnings);
  connect(mwUi->actionJobQueueAcknowledgeAllErrors,         &QAction::triggered,                              m_model, &Model::acknowledgeAllErrors);
  connect(mwUi->actionJobQueueAcknowledgeAllWarningsErrors, &QAction::triggered,                              m_model, &Model::acknowledgeAllWarnings);
  connect(mwUi->actionJobQueueAcknowledgeAllWarningsErrors, &QAction::triggered,                              m_model, &Model::acknowledgeAllErrors);

  connect(m_startAutomaticallyAction,                       &QAction::triggered,                              this,    &Tool::onStartAutomatically);
  connect(m_startManuallyAction,                            &QAction::triggered,                              this,    &Tool::onStartManually);
  connect(m_viewOutputAction,                               &QAction::triggered,                              this,    &Tool::onViewOutput);
  connect(m_removeAction,                                   &QAction::triggered,                              this,    &Tool::onRemove);
  connect(m_acknowledgeSelectedWarningsAction,              &QAction::triggered,                              this,    &Tool::acknowledgeSelectedWarnings);
  connect(m_acknowledgeSelectedErrorsAction,                &QAction::triggered,                              this,    &Tool::acknowledgeSelectedErrors);
  connect(m_acknowledgeSelectedWarningsErrorsAction,        &QAction::triggered,                              this,    &Tool::acknowledgeSelectedWarnings);
  connect(m_acknowledgeSelectedWarningsErrorsAction,        &QAction::triggered,                              this,    &Tool::acknowledgeSelectedErrors);
  connect(m_openFolderAction,                               &QAction::triggered,                              this,    &Tool::onOpenFolder);
  connect(m_editAndRemoveAction,                            &QAction::triggered,                              this,    &Tool::onEditAndRemove);
  connect(m_startImmediatelyAction,                         &QAction::triggered,                              this,    &Tool::onStartImmediately);

  connect(ui->jobs->selectionModel(),                       &QItemSelectionModel::selectionChanged,           this,    &Tool::enableMoveJobsButtons);
  connect(ui->jobs,                                         &Util::BasicTreeView::doubleClicked,              this,    &Tool::onViewOutput);
  connect(ui->jobs,                                         &Util::BasicTreeView::customContextMenuRequested, this,    &Tool::onContextMenu);
  connect(ui->jobs,                                         &Util::BasicTreeView::deletePressed,              this,    &Tool::onRemove);
  connect(ui->jobs,                                         &Util::BasicTreeView::ctrlDownPressed,            this,    [this]() { moveJobsUpOrDown(false); });
  connect(ui->jobs,                                         &Util::BasicTreeView::ctrlUpPressed,              this,    [this]() { moveJobsUpOrDown(true); });
  connect(ui->jobs->header(),                               &QHeaderView::sortIndicatorChanged,               this,    &Tool::sortJobs);
  connect(ui->moveJobsDown,                                 &QPushButton::clicked,                            this,    [this]() { moveJobsUpOrDown(false); });
  connect(ui->moveJobsUp,                                   &QPushButton::clicked,                            this,    [this]() { moveJobsUpOrDown(true); });
  connect(m_model,                                          &Model::orderChanged,                             this,    &Tool::hideSortIndicator);

  connect(mw,                                               &MainWindow::preferencesChanged,                  this,    &Tool::retranslateUi);
  connect(mw,                                               &MainWindow::preferencesChanged,                  this,    &Tool::setupMoveJobsButtons);
  connect(mw,                                               &MainWindow::aboutToClose,                        m_model, &Model::removeCompletedJobsBeforeExiting);
  connect(mw,                                               &MainWindow::aboutToClose,                        m_model, &Model::saveJobs);

  connect(MainWindow::watchCurrentJobTab(),                 &WatchJobs::Tab::watchCurrentJobTabCleared,       m_model, &Model::resetTotalProgress);
}

void
Tool::setupMoveJobsButtons() {
  ui->moveJobsButtons->setVisible(Util::Settings::get().m_showMoveUpDownButtons);
  enableMoveJobsButtons();
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
  auto hasNotRunning = false;
  auto hasEditable   = false;
  auto numSelected   = 0;

  m_model->withSelectedJobs(ui->jobs, [&numSelected, &hasNotRunning, &hasEditable](Job const &job) {
    ++numSelected;
    if (Job::Running != job.status())
      hasNotRunning = true;

    if (job.isEditable())
      hasEditable = true;
  });

  auto hasSelection = !!numSelected;

  m_startAutomaticallyAction->setEnabled(hasNotRunning);
  m_startManuallyAction->setEnabled(hasNotRunning);
  m_viewOutputAction->setEnabled(hasSelection);
  m_removeAction->setEnabled(hasSelection);
  m_openFolderAction->setEnabled(hasSelection);
  m_editAndRemoveAction->setEnabled(hasEditable);
  m_startImmediatelyAction->setEnabled(hasNotRunning);

  m_acknowledgeSelectedWarningsAction->setEnabled(hasSelection);
  m_acknowledgeSelectedErrorsAction->setEnabled(hasSelection);
  m_acknowledgeSelectedWarningsErrorsAction->setEnabled(hasSelection);

  m_startAutomaticallyAction->setText(QNY("&Start job automatically", "&Start jobs automatically", numSelected));
  m_startManuallyAction->setText(QNY("Start job &manually", "Start jobs &manually", numSelected));
  m_startImmediatelyAction->setText(QNY("Start job &immediately", "Start jobs &immediately", numSelected));
  m_removeAction->setText(QNY("&Remove job", "&Remove jobs", numSelected));

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

  if (idsToRemove.isEmpty())
    return;

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
  m_model->removeJobsIf([](Job const &job) {
      return (Job::DoneOk       == job.status())
          || (Job::DoneWarnings == job.status())
          || (Job::Failed       == job.status())
          || (Job::Aborted      == job.status());
    });
}

void
Tool::onRemoveDoneOk() {
  m_model->removeJobsIf([](Job const &job) { return Job::DoneOk == job.status(); });
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
Tool::moveJobsUpOrDown(bool up) {
  auto focus = App::instance()->focusWidget();

  m_model->withSelectedJobsAsList(ui->jobs, [this, up](auto const &selectedJobs) {
    m_model->moveJobsUpOrDown(selectedJobs, up);
    this->selectJobs(selectedJobs);
  });

  if (focus)
    focus->setFocus();

  ui->jobs->scrollToFirstSelected();
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
Tool::addJob(JobPtr const &job) {
  job->saveQueueFile();
  m_model->add(job);
}

bool
Tool::addJobFile(QString const &fileName) {
  try {
    auto job = Job::loadJob(fileName);
    if (!job)
      throw Merge::InvalidSettingsX{};

    auto muxJob = std::dynamic_pointer_cast<MuxJob>(job);
    if (muxJob) {
      job = std::make_shared<MuxJob>(Job::PendingManual, std::make_shared<Merge::MuxConfig>(muxJob->config()));
      job->setDescription(muxJob->description());

    } else
      throw Merge::InvalidSettingsX{};

    job->setDateAdded(QDateTime::currentDateTime());
    if (job->description().isEmpty())
      job->setDescription(job->displayableDescription());

    addJob(job);

    return true;

  } catch (Merge::InvalidSettingsX &) {
  }

  return false;
}

bool
Tool::addDroppedFileAsJob(QString const &fileName) {
  if (addJobFile(fileName))
    return true;

  try {
    auto muxConfig = Merge::MuxConfig::loadSettings(fileName);
    if (!muxConfig)
      throw Merge::InvalidSettingsX{};

    auto job = std::make_shared<Jobs::MuxJob>(Job::PendingManual, muxConfig);

    job->setDateAdded(QDateTime::currentDateTime());
    job->setDescription(job->displayableDescription());

    addJob(job);

    return true;

  } catch (Merge::InvalidSettingsX &) {
  }

  return false;
}

void
Tool::retranslateUi() {
  ui->retranslateUi(this);
  m_model->retranslateUi();

  m_viewOutputAction->setText(QY("&View output"));
  m_openFolderAction->setText(QY("&Open folder"));
  m_editAndRemoveAction->setText(QY("E&dit in corresponding tool and remove from queue"));

  m_acknowledgeSelectedWarningsAction->setText(QY("Acknowledge &warnings"));
  m_acknowledgeSelectedErrorsAction->setText(QY("Acknowledge &errors"));
  m_acknowledgeSelectedWarningsErrorsAction->setText(QY("&Acknowledge warnings and errors")); // b

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

    else if (jobToEdit.isEditable()) {
      auto idToEdit = jobToEdit.id();

      openJobInTool(jobToEdit);
      m_model->removeJobsIf([idToEdit](Job const &jobToRemove) { return idToEdit == jobToRemove.id(); });
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
              ->title(QY("Abort running jobs"))
              .text(QY("Do you really want to abort all currently running jobs?"))
              .buttonLabel(QMessageBox::Yes, QY("&Abort jobs"))
              .buttonLabel(QMessageBox::No,  QY("Cancel"))
              .exec() == QMessageBox::Yes))
        job.abort();

      askedBeforeAborting = true;
    });
  });
}

void
Tool::dragEnterEvent(QDragEnterEvent *event) {
  m_filesDDHandler.handle(event, false);
}

void
Tool::dropEvent(QDropEvent *event) {
  if (m_filesDDHandler.handle(event, true)) {
    m_droppedFiles += m_filesDDHandler.fileNames();
    QTimer::singleShot(0, this, [this]() { processDroppedFiles(); });
  }
}

void
Tool::processDroppedFiles() {
  auto fileNames = m_droppedFiles;
  m_droppedFiles.clear();

  for (auto const &fileName : fileNames)
    if (!addDroppedFileAsJob(fileName))
      Util::MessageBox::critical(this)->title(QY("Error loading settings file")).text(QY("The file '%1' is neither a job queue file nor a settings file.").arg(fileName)).exec();
}

void
Tool::selectJobs(QList<Job *> const &jobs) {
  auto numColumns = m_model->columnCount() - 1;
  auto selection  = QItemSelection{};

  for (auto const &job : jobs) {
    auto row = m_model->rowFromId(job->id());
    selection.select(m_model->index(row, 0), m_model->index(row, numColumns));
  }

  ui->jobs->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
}

void
Tool::enableMoveJobsButtons() {
  auto hasSelected = false;
  m_model->withSelectedJobsAsList(ui->jobs, [&hasSelected](auto const &selectedJobs) { hasSelected = !selectedJobs.isEmpty(); });

  ui->moveJobsUp->setEnabled(hasSelected);
  ui->moveJobsDown->setEnabled(hasSelected);
}

bool
Tool::checkIfOverwritingExistingFileIsOK(QString const &existingDestination) {
  if (!Util::Settings::get().m_warnBeforeOverwriting)
    return true;

  auto answer = Util::MessageBox::question(this)
    ->title(QY("Overwrite existing file"))
    .text(Q("%1 %2")
          .arg(QY("The file '%1' exists already and might be overwritten depending on the configuration & the content of the source files.").arg(existingDestination))
          .arg(QY("Do you want to continue and risk overwriting the file?")))
    .buttonLabel(QMessageBox::Yes, QY("C&ontinue"))
    .buttonLabel(QMessageBox::No,  QY("Cancel"))
    .exec();

  return answer == QMessageBox::Yes;
}

bool
Tool::checkIfOverwritingExistingJobIsOK(QString const &newDestination,
                                        bool isMuxJobAndSplittingEnabled) {
  if (!Util::Settings::get().m_warnBeforeOverwriting)
    return true;

  auto jobWithSameDestinationExists = false;
  auto nativeNewDestination         = QDir::toNativeSeparators(newDestination);

  m_model->withAllJobs([&jobWithSameDestinationExists, &nativeNewDestination, isMuxJobAndSplittingEnabled](Jobs::Job &job) {
    auto jobDestination        = job.destinationFileName();
    auto jobIsSplittingEnabled = dynamic_cast<Jobs::MuxJob *>(&job) && static_cast<Jobs::MuxJob &>(job).config().isSplittingEnabled();

    if (   jobDestination.isEmpty()
        || !mtx::included_in(job.status(), Jobs::Job::PendingManual, Jobs::Job::PendingAuto, Jobs::Job::Running))
      return;

    if (   (QDir::toNativeSeparators(jobDestination) == nativeNewDestination)
        && (jobIsSplittingEnabled                    == isMuxJobAndSplittingEnabled))
      jobWithSameDestinationExists = true;
  });

  if (!jobWithSameDestinationExists)
    return true;

  auto answer = Util::MessageBox::question(this)
    ->title(QY("Overwrite existing file"))
    .text(Q("%1 %2 %3")
          .arg(QY("A job creating the file '%1' is already in the job queue.").arg(newDestination))
          .arg(QY("If you add another job with the same destination file then file created before will be overwritten."))
          .arg(QY("Do you want to overwrite the file?")))
    .buttonLabel(QMessageBox::Yes, QY("&Overwrite file"))
    .buttonLabel(QMessageBox::No,  QY("Cancel"))
    .exec();

  return answer == QMessageBox::Yes;
}

void
Tool::sortJobs(int logicalColumnIndex,
               Qt::SortOrder order) {
  m_model->withSelectedJobsAsList(ui->jobs, [this, logicalColumnIndex, order](auto const &selectedJobs) {
    m_model->sortAllJobs(logicalColumnIndex, order);
    this->selectJobs(selectedJobs);
  });

  ui->jobs->scrollToFirstSelected();

  ui->jobs->header()->setSortIndicatorShown(true);
}

void
Tool::hideSortIndicator() {
  ui->jobs->header()->setSortIndicatorShown(false);
}

}
