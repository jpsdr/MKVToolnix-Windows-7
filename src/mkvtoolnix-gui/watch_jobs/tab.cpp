#include "common/common_pch.h"

#include <QCursor>
#include <QDebug>
#include <QFileDialog>
#include <QMenu>
#include <QPushButton>
#include <QtGlobal>

#include "common/list_utils.h"
#include "common/qt.h"
#include "common/strings/formatting.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/forms/watch_jobs/tab.h"
#include "mkvtoolnix-gui/jobs/mux_job.h"
#include "mkvtoolnix-gui/jobs/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/util/file_dialog.h"
#include "mkvtoolnix-gui/util/message_box.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/string.h"
#include "mkvtoolnix-gui/util/widget.h"
#include "mkvtoolnix-gui/watch_jobs/tab.h"
#include "mkvtoolnix-gui/watch_jobs/tool.h"

#define MTX_RUN_PROGRAM_CONFIGURATION_ADDRESS   "mtxRunProgramConfigurationAddress"
#define MTX_RUN_PROGRAM_CONFIGURATION_CONDITION "mtxRunProgramConfigurationCondition"

using namespace mtx::gui;

namespace mtx { namespace gui { namespace WatchJobs {

class TabPrivate {
  friend class Tab;

  // UI stuff:
  std::unique_ptr<Ui::Tab> ui;
  QStringList m_fullOutput;
  uint64_t m_id, m_currentJobProgress, m_queueProgress;
  QHash<Jobs::Job::LineType, bool> m_currentJobLineTypeSeen;
  Jobs::Job::Status m_currentJobStatus;
  QDateTime m_currentJobStartTime;
  QString m_currentJobDescription;
  QMenu *m_whenFinished, *m_moreActions;
  bool m_forCurrentJob;

  // Only use this variable for determining whether or not to ignore
  // certain signals.
  QObject const *m_currentlyConnectedJob;

  QAction *m_saveOutputAction, *m_clearOutputAction, *m_openFolderAction;

  explicit TabPrivate(Tab *tab, bool forCurrentJob)
    : ui{new Ui::Tab}
    , m_id{std::numeric_limits<uint64_t>::max()}
    , m_currentJobProgress{}
    , m_queueProgress{}
    , m_currentJobStatus{Jobs::Job::PendingManual}
    , m_whenFinished{new QMenu{tab}}
    , m_moreActions{new QMenu{tab}}
    , m_forCurrentJob{forCurrentJob}
    , m_currentlyConnectedJob{}
    , m_saveOutputAction{new QAction{tab}}
    , m_clearOutputAction{new QAction{tab}}
    , m_openFolderAction{new QAction{tab}}
  {
  }
};

Tab::Tab(QWidget *parent,
         bool forCurrentJob)
  : QWidget{parent}
  , d_ptr{new TabPrivate{this, forCurrentJob}}
{
  setupUi();
}

Tab::~Tab() {
}

void
Tab::setupUi() {
  Q_D(Tab);

  // Setup UI controls.
  d->ui->setupUi(this);

  d->m_saveOutputAction->setEnabled(false);

  Util::preventScrollingWithoutFocus(this);

  auto &cfg = Util::Settings::get();
  for (auto const &splitter : findChildren<QSplitter *>())
    cfg.handleSplitterSizes(splitter);

  setupMoreActionsMenu();

  retranslateUi();

  d->ui->acknowledgeWarningsAndErrorsButton->setIcon(QIcon{Q(":/icons/16x16/dialog-ok-apply.png")});
  d->ui->abortButton->setIcon(QIcon{Q(":/icons/16x16/dialog-cancel.png")});

  auto model = MainWindow::jobTool()->model();

  connect(d->ui->abortButton,                        &QPushButton::clicked,                                  this, &Tab::onAbort);
  connect(d->ui->acknowledgeWarningsAndErrorsButton, &QPushButton::clicked,                                  this, &Tab::acknowledgeWarningsAndErrors);
  connect(model,                                     &Jobs::Model::progressChanged,                          this, &Tab::onQueueProgressChanged);
  connect(model,                                     &Jobs::Model::queueStatusChanged,                       this, &Tab::updateRemainingTime);
  connect(d->m_whenFinished,                         &QMenu::aboutToShow,                                    this, &Tab::setupWhenFinishedActions);
  connect(d->m_moreActions,                          &QMenu::aboutToShow,                                    this, &Tab::enableMoreActionsActions);
  connect(d->m_saveOutputAction,                     &QAction::triggered,                                    this, &Tab::onSaveOutput);
  connect(d->m_clearOutputAction,                    &QAction::triggered,                                    this, &Tab::clearOutput);
  connect(d->m_openFolderAction,                     &QAction::triggered,                                    this, &Tab::openFolder);
  connect(MainWindow::jobTool()->model(),            &Jobs::Model::numUnacknowledgedWarningsOrErrorsChanged, this, &Tab::disableButtonIfAllWarningsAndErrorsButtonAcknowledged);
}

void
Tab::setupMoreActionsMenu() {
  Q_D(Tab);

  d->ui->whenFinishedButton->setMenu(d->m_whenFinished);

  // Setup the "more actions" menu.
  d->m_moreActions->addAction(d->m_openFolderAction);
  d->m_moreActions->addSeparator();
  d->m_moreActions->addAction(d->m_saveOutputAction);

  if (isCurrentJobTab())
    d->m_moreActions->addAction(d->m_clearOutputAction);

  d->ui->moreActionsButton->setMenu(d->m_moreActions);

  d->m_openFolderAction->setIcon(QIcon{Q(":/icons/16x16/document-open-folder.png")});
  d->m_saveOutputAction->setIcon(QIcon{Q(":/icons/16x16/document-save.png")});
}

void
Tab::retranslateUi() {
  Q_D(Tab);

  d->ui->retranslateUi(this);

  d->ui->description->setText(d->m_currentJobDescription.isEmpty() ? QY("No job has been started yet.") : d->m_currentJobDescription);
  d->m_saveOutputAction->setText(QY("&Save output"));
  d->m_clearOutputAction->setText(QY("&Clear output and reset progress"));
  d->m_openFolderAction->setText(QY("&Open folder"));

  d->ui->output->setPlaceholderText(QY("No output yet"));
  d->ui->warnings->setPlaceholderText(QY("No warnings yet"));
  d->ui->errors->setPlaceholderText(QY("No errors yet"));
}

void
Tab::connectToJob(Jobs::Job const &job) {
  Q_D(Tab);

  d->m_currentlyConnectedJob = &job;
  d->m_id                    = job.id();
  auto connType              = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(&job, &Jobs::Job::statusChanged,   this, &Tab::onStatusChanged,      connType);
  connect(&job, &Jobs::Job::progressChanged, this, &Tab::onJobProgressChanged, connType);
  connect(&job, &Jobs::Job::lineRead,        this, &Tab::onLineRead,           connType);
}

void
Tab::disconnectFromJob(Jobs::Job const &job) {
  Q_D(Tab);

  if (d->m_currentlyConnectedJob == &job) {
    d->m_currentlyConnectedJob = nullptr;
    d->m_id                    = std::numeric_limits<uint64_t>::max();
  }

  disconnect(&job, &Jobs::Job::statusChanged,   this, &Tab::onStatusChanged);
  disconnect(&job, &Jobs::Job::progressChanged, this, &Tab::onJobProgressChanged);
  disconnect(&job, &Jobs::Job::lineRead,        this, &Tab::onLineRead);
}

uint64_t
Tab::queueProgress()
  const {
  Q_D(const Tab);

  return d->m_queueProgress;
}

void
Tab::onAbort() {
  Q_D(Tab);

  if (std::numeric_limits<uint64_t>::max() == d->m_id)
    return;

  if (   Util::Settings::get().m_warnBeforeAbortingJobs
      && (Util::MessageBox::question(this)
            ->title(QY("Abort running job"))
            .text(QY("Do you really want to abort this job?"))
            .buttonLabel(QMessageBox::Yes, QY("&Abort job"))
            .buttonLabel(QMessageBox::No,  QY("Cancel"))
            .exec() == QMessageBox::No))
    return;

  MainWindow::jobTool()->model()->withJob(d->m_id, [](Jobs::Job &job) { job.abort(); });
}

void
Tab::onStatusChanged(uint64_t id,
                     mtx::gui::Jobs::Job::Status,
                     mtx::gui::Jobs::Job::Status newStatus) {
  Q_D(Tab);

  if (QObject::sender() != d->m_currentlyConnectedJob)
    return;

  auto job = MainWindow::jobTool()->model()->fromId(id);
  if (!job) {
    d->ui->abortButton->setEnabled(false);
    d->m_saveOutputAction->setEnabled(false);
    MainWindow::watchJobTool()->enableMenuActions();

    d->m_id                 = std::numeric_limits<uint64_t>::max();
    d->m_currentJobProgress = 0;
    d->m_currentJobProgress = Jobs::Job::Aborted;
    updateRemainingTime();

    return;
  }

  job->action([this, d, job, newStatus]() {
    d->m_currentJobStatus = job->status();

    d->ui->abortButton->setEnabled(Jobs::Job::Running == d->m_currentJobStatus);
    d->m_saveOutputAction->setEnabled(true);
    d->ui->status->setText(Jobs::Job::displayableStatus(d->m_currentJobStatus));
    MainWindow::watchJobTool()->enableMenuActions();

    // Check for the signalled status, not the current one, in order to
    // detect a change from "not running" to "running" only once, no
    // matter which order the signals arrive in.
    if (Jobs::Job::Running == newStatus)
      setInitialDisplay(*job);

    else if (mtx::included_in(d->m_currentJobStatus, Jobs::Job::DoneOk, Jobs::Job::DoneWarnings, Jobs::Job::Failed, Jobs::Job::Aborted))
      d->ui->finishedAt->setText(Util::displayableDate(job->dateFinished()));
  });

  updateRemainingTime();
}

void
Tab::updateOneRemainingTimeLabel(QLabel *label,
                                 QDateTime const &startTime,
                                 uint64_t progress) {
  if (!progress)
    return;

  auto elapsedDuration = startTime.msecsTo(QDateTime::currentDateTime());
  if (5000 > elapsedDuration)
    label->setText(Q("–"));

  else {
    auto totalDuration     = elapsedDuration * 100 / progress;
    auto remainingDuration = totalDuration - elapsedDuration;
    label->setText(Q(create_minutes_seconds_time_string(remainingDuration / 1000)));
  }
}

void
Tab::updateRemainingTime() {
  Q_D(Tab);

  if ((Jobs::Job::Running != d->m_currentJobStatus) || !d->m_currentJobProgress)
    d->ui->remainingTimeCurrentJob->setText(Q("–"));

  else
    updateOneRemainingTimeLabel(d->ui->remainingTimeCurrentJob, d->m_currentJobStartTime, d->m_currentJobProgress);

  auto model = MainWindow::jobTool()->model();
  if (!model->isRunning())
    d->ui->remainingTimeQueue->setText(Q("–"));

  else
    updateOneRemainingTimeLabel(d->ui->remainingTimeQueue, model->queueStartTime(), d->m_queueProgress);
}

void
Tab::onQueueProgressChanged(int,
                            int totalProgress) {
  Q_D(Tab);

  d->m_queueProgress = totalProgress;
  updateRemainingTime();
}

void
Tab::onJobProgressChanged(uint64_t,
                          unsigned int progress) {
  Q_D(Tab);

  if (QObject::sender() != d->m_currentlyConnectedJob)
    return;

  d->ui->progressBar->setValue(progress);
  d->m_currentJobProgress = progress;
  updateRemainingTime();
}

void
Tab::onLineRead(QString const &line,
                Jobs::Job::LineType type) {
  Q_D(Tab);

  if ((QObject::sender() != d->m_currentlyConnectedJob) || line.isEmpty())
    return;

  auto &storage = Jobs::Job::InfoLine    == type ? d->ui->output
                : Jobs::Job::WarningLine == type ? d->ui->warnings
                :                                  d->ui->errors;

  auto prefix   = Jobs::Job::InfoLine    == type ? Q("")
                : Jobs::Job::WarningLine == type ? Q("%1 ").arg(QY("Warning:"))
                :                                  Q("%1 ").arg(QY("Error:"));

  if (mtx::included_in(type, Jobs::Job::WarningLine, Jobs::Job::ErrorLine)) {
    d->ui->acknowledgeWarningsAndErrorsButton->setEnabled(true);

    if (isCurrentJobTab() && Util::Settings::get().m_showOutputOfAllJobs && !d->m_currentJobLineTypeSeen[type]) {
      d->m_currentJobLineTypeSeen[type] = true;
      auto dateStarted                  = Util::displayableDate(d->m_currentJobStartTime);
      auto separator                    = Jobs::Job::WarningLine == type ? QY("--- Warnings emitted by job '%1' started on %2 ---").arg(d->m_currentJobDescription).arg(dateStarted)
                                        :                                  QY("--- Errors emitted by job '%1' started on %2 ---"  ).arg(d->m_currentJobDescription).arg(dateStarted);

      storage->appendPlainText(separator);
    }
  }

  d->m_fullOutput << Q("%1%2").arg(prefix).arg(line);
  storage->appendPlainText(line);

}

void
Tab::setInitialDisplay(Jobs::Job const &job) {
  Q_D(Tab);

  auto dateStarted           = Util::displayableDate(job.dateStarted());
  d->m_currentJobDescription = job.description();

  if (isCurrentJobTab() && Util::Settings::get().m_showOutputOfAllJobs) {
    auto outputOfJobLine = QY("--- Output of job '%1' started on %2 ---").arg(d->m_currentJobDescription).arg(dateStarted);
    d->m_fullOutput << outputOfJobLine << job.fullOutput();

    d->ui->output->appendPlainText(outputOfJobLine);

  } else {
    d->m_fullOutput = job.fullOutput();

    d->ui->output  ->setPlainText(!job.output().isEmpty()   ? Q("%1\n").arg(job.output().join("\n"))   : Q(""));
    d->ui->warnings->setPlainText(!job.warnings().isEmpty() ? Q("%1\n").arg(job.warnings().join("\n")) : Q(""));
    d->ui->errors  ->setPlainText(!job.errors().isEmpty()   ? Q("%1\n").arg(job.errors().join("\n"))   : Q(""));
  }

  d->m_currentJobLineTypeSeen.clear();

  d->m_currentJobStatus    = job.status();
  d->m_currentJobProgress  = job.progress();
  d->m_currentJobStartTime = job.dateStarted();
  d->m_queueProgress       = MainWindow::watchCurrentJobTab()->queueProgress();

  d->ui->description->setText(d->m_currentJobDescription);
  d->ui->status->setText(Jobs::Job::displayableStatus(job.status()));
  d->ui->progressBar->setValue(job.progress());

  d->ui->startedAt ->setText(job.dateStarted() .isValid() ? Util::displayableDate(job.dateStarted())  : QY("Not started yet"));
  d->ui->finishedAt->setText(job.dateFinished().isValid() ? Util::displayableDate(job.dateFinished()) : QY("Not finished yet"));

  d->ui->abortButton->setEnabled(Jobs::Job::Running == job.status());
  d->m_saveOutputAction->setEnabled(!mtx::included_in(job.status(), Jobs::Job::PendingManual, Jobs::Job::PendingAuto, Jobs::Job::Disabled));

  d->ui->acknowledgeWarningsAndErrorsButton->setEnabled(job.numUnacknowledgedWarnings() || job.numUnacknowledgedErrors());

  updateRemainingTime();
}

void
Tab::disableButtonIfAllWarningsAndErrorsButtonAcknowledged(int numWarnings,
                                                           int numErrors) {
  Q_D(Tab);

  if (!numWarnings && !numErrors)
    d->ui->acknowledgeWarningsAndErrorsButton->setEnabled(false);
}

void
Tab::onSaveOutput() {
  Q_D(Tab);

  auto &cfg     = Util::Settings::get();
  auto fileName = Util::getSaveFileName(this, QY("Save job output"), cfg.lastOpenDirPath(), QY("Text files") + Q(" (*.txt);;") + QY("All files") + Q(" (*)"));

  if (fileName.isEmpty())
    return;

  QFile out{fileName};
  if (out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    out.write(Q("%1\n").arg(d->m_fullOutput.join(Q("\n"))).toUtf8());
    out.close();
  }

  cfg.m_lastOpenDir = QFileInfo{fileName}.path();
  cfg.save();
}

uint64_t
Tab::id()
  const {
  Q_D(const Tab);

  return d->m_id;
}

bool
Tab::isSaveOutputEnabled()
  const {
  Q_D(const Tab);

  return d->m_saveOutputAction->isEnabled();
}

bool
Tab::isCurrentJobTab()
  const {
  Q_D(const Tab);

  return d->m_forCurrentJob;
}

void
Tab::acknowledgeWarningsAndErrors() {
  Q_D(Tab);

  MainWindow::jobTool()->acknowledgeWarningsAndErrors(d->m_id);
  d->ui->acknowledgeWarningsAndErrorsButton->setEnabled(false);
}

void
Tab::clearOutput() {
  Q_D(Tab);

  d->ui->output->clear();
  d->ui->warnings->clear();
  d->ui->errors->clear();

  if (MainWindow::jobTool()->model()->isRunning())
    return;

  d->m_currentJobDescription.clear();

  d->ui->progressBar->reset();
  d->ui->status->setText(QY("No job started yet"));
  d->ui->description->setText(QY("No job has been started yet."));
  d->ui->startedAt->setText(QY("Not started yet"));
  d->ui->finishedAt->setText(QY("Not finished yet"));
  d->ui->remainingTimeCurrentJob->setText(Q("–"));
  d->ui->remainingTimeQueue->setText(Q("–"));

  emit watchCurrentJobTabCleared();
}

void
Tab::openFolder() {
  Q_D(Tab);

  MainWindow::jobTool()->model()->withJob(d->m_id, [](Jobs::Job &job) { job.openOutputFolder(); });
}

void
Tab::enableMoreActionsActions() {
  Q_D(Tab);

  auto hasJob = std::numeric_limits<uint64_t>::max() != d->m_id;
  d->m_openFolderAction->setEnabled(hasJob);
}

void
Tab::setupWhenFinishedActions() {
  Q_D(Tab);

  d->m_whenFinished->clear();

  auto afterCurrentJobMenu                = new QMenu{QY("Execute action after next &job completion")};
  auto afterJobQueueMenu                  = new QMenu{QY("Execute action when the &queue completes")};
  auto editRunProgramConfigurationsAction = new QAction{d->m_whenFinished};
  auto menus                              = QVector<QMenu *>{} << afterCurrentJobMenu << afterJobQueueMenu;
  auto &programRunner                     = App::programRunner();

  editRunProgramConfigurationsAction->setText(QY("&Edit available actions to execute"));

  d->m_whenFinished->addMenu(afterCurrentJobMenu);
  d->m_whenFinished->addMenu(afterJobQueueMenu);
  d->m_whenFinished->addSeparator();
  d->m_whenFinished->addAction(editRunProgramConfigurationsAction);

  connect(editRunProgramConfigurationsAction, &QAction::triggered, MainWindow::get(), &MainWindow::editRunProgramConfigurations);

  for (auto const &config : Util::Settings::get().m_runProgramConfigurations) {
    if (!config->m_active)
      continue;

    for (auto const &menu : menus) {
      auto action    = menu->addAction(config->name());
      auto forQueue  = menu == afterJobQueueMenu;
      auto condition = forQueue ? Jobs::ProgramRunner::ExecuteActionCondition::AfterQueueFinishes : Jobs::ProgramRunner::ExecuteActionCondition::AfterJobFinishes;

      action->setProperty(MTX_RUN_PROGRAM_CONFIGURATION_ADDRESS,   reinterpret_cast<quint64>(config.get()));
      action->setProperty(MTX_RUN_PROGRAM_CONFIGURATION_CONDITION, static_cast<int>(condition));

      action->setCheckable(true);
      action->setChecked(programRunner.isActionToExecuteEnabled(*config, condition));

      connect(action, &QAction::triggered, this, &Tab::toggleActionToExecute);
    }
  }

  afterCurrentJobMenu->setEnabled(!afterCurrentJobMenu->isEmpty());
  afterJobQueueMenu->setEnabled(!afterJobQueueMenu->isEmpty());
}

void
Tab::toggleActionToExecute() {
  auto &action      = *qobject_cast<QAction *>(sender());
  auto config       = reinterpret_cast<Util::Settings::RunProgramConfig *>(action.property(MTX_RUN_PROGRAM_CONFIGURATION_ADDRESS).value<quint64>());
  auto conditionIdx = action.property(MTX_RUN_PROGRAM_CONFIGURATION_CONDITION).value<int>();
  auto condition    = conditionIdx == static_cast<int>(Jobs::ProgramRunner::ExecuteActionCondition::AfterQueueFinishes) ? Jobs::ProgramRunner::ExecuteActionCondition::AfterQueueFinishes
                    :                                                                                                     Jobs::ProgramRunner::ExecuteActionCondition::AfterJobFinishes;
  auto enable       = action.isChecked();

  App::programRunner().enableActionToExecute(*config, condition, enable);
}

}}}
