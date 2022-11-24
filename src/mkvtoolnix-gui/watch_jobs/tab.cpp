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
#include "mkvtoolnix-gui/util/date_time.h"
#include "mkvtoolnix-gui/util/file.h"
#include "mkvtoolnix-gui/util/file_dialog.h"
#include "mkvtoolnix-gui/util/message_box.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/widget.h"
#include "mkvtoolnix-gui/watch_jobs/tab.h"
#include "mkvtoolnix-gui/watch_jobs/tool.h"

constexpr auto MTX_RUN_PROGRAM_CONFIGURATION_ADDRESS   = "mtxRunProgramConfigurationAddress";
constexpr auto MTX_RUN_PROGRAM_CONFIGURATION_CONDITION = "mtxRunProgramConfigurationCondition";

using namespace mtx::gui;

namespace mtx::gui::WatchJobs {

class TabPrivate {
  friend class Tab;

  // UI stuff:
  std::unique_ptr<Ui::Tab> ui;
  QStringList m_fullOutput;
  std::optional<uint64_t> m_id;
  uint64_t m_currentJobProgress, m_queueProgress;
  QHash<Jobs::Job::LineType, bool> m_currentJobLineTypeSeen;
  Jobs::Job::Status m_currentJobStatus;
  QDateTime m_currentJobStartTime;
  QString m_currentJobDescription;
  QMenu *m_whenFinished, *m_moreActions;
  bool m_forCurrentJob;

  // Only use this variable for determining whether or not to ignore
  // certain Q_SIGNALS.
  QObject const *m_currentlyConnectedJob;

  QAction *m_saveOutputAction, *m_clearOutputAction, *m_openFolderAction;

  explicit TabPrivate(Tab *tab, bool forCurrentJob)
    : ui{new Ui::Tab}
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
  , p_ptr{new TabPrivate{this, forCurrentJob}}
{
  setupUi();
}

Tab::~Tab() {
}

void
Tab::setupUi() {
  auto p = p_func();

  // Setup UI controls.
  p->ui->setupUi(this);

  p->m_saveOutputAction->setEnabled(false);

  Util::preventScrollingWithoutFocus(this);

  auto &cfg = Util::Settings::get();
  for (auto const &splitter : findChildren<QSplitter *>())
    cfg.handleSplitterSizes(splitter);

  setupMoreActionsMenu();

  retranslateUi();

  p->ui->acknowledgeWarningsAndErrorsButton->setIcon(QIcon::fromTheme(Q("dialog-ok-apply")));
  p->ui->abortButton->setIcon(QIcon::fromTheme(Q("dialog-cancel")));

  auto model = MainWindow::jobTool()->model();

  connect(p->ui->abortButton,                        &QPushButton::clicked,                                  this, &Tab::onAbort);
  connect(p->ui->acknowledgeWarningsAndErrorsButton, &QPushButton::clicked,                                  this, &Tab::acknowledgeWarningsAndErrors);
  connect(model,                                     &Jobs::Model::progressChanged,                          this, &Tab::onQueueProgressChanged);
  connect(model,                                     &Jobs::Model::queueStatusChanged,                       this, &Tab::updateRemainingTime);
  connect(p->m_whenFinished,                         &QMenu::aboutToShow,                                    this, &Tab::setupWhenFinishedActions);
  connect(p->m_moreActions,                          &QMenu::aboutToShow,                                    this, &Tab::enableMoreActionsActions);
  connect(p->m_saveOutputAction,                     &QAction::triggered,                                    this, &Tab::onSaveOutput);
  connect(p->m_clearOutputAction,                    &QAction::triggered,                                    this, &Tab::clearOutput);
  connect(p->m_openFolderAction,                     &QAction::triggered,                                    this, &Tab::openFolder);
  connect(MainWindow::jobTool()->model(),            &Jobs::Model::numUnacknowledgedWarningsOrErrorsChanged, this, &Tab::disableButtonIfAllWarningsAndErrorsButtonAcknowledged);
}

void
Tab::setupMoreActionsMenu() {
  auto p = p_func();

  p->ui->whenFinishedButton->setMenu(p->m_whenFinished);

  // Setup the "more actions" menu.
  p->m_moreActions->addAction(p->m_openFolderAction);
  p->m_moreActions->addSeparator();
  p->m_moreActions->addAction(p->m_saveOutputAction);

  if (isCurrentJobTab())
    p->m_moreActions->addAction(p->m_clearOutputAction);

  p->ui->moreActionsButton->setMenu(p->m_moreActions);

  p->m_openFolderAction->setIcon(QIcon::fromTheme(Q("document-open-folder")));
  p->m_saveOutputAction->setIcon(QIcon::fromTheme(Q("document-save")));
}

void
Tab::retranslateUi() {
  auto p = p_func();

  p->ui->retranslateUi(this);

  p->ui->description->setText(p->m_currentJobDescription.isEmpty() ? QY("No job has been started yet.") : p->m_currentJobDescription);
  p->m_saveOutputAction->setText(QY("&Save output"));
  p->m_clearOutputAction->setText(QY("&Clear output and reset progress"));
  p->m_openFolderAction->setText(QY("&Open folder"));

  p->ui->output->setPlaceholderText(QY("No output yet"));
  p->ui->warnings->setPlaceholderText(QY("No warnings yet"));
  p->ui->errors->setPlaceholderText(QY("No errors yet"));
}

void
Tab::connectToJob(Jobs::Job const &job) {
  auto p                     = p_func();
  p->m_currentlyConnectedJob = &job;
  p->m_id                    = job.id();
  auto connType              = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(&job, &Jobs::Job::statusChanged,   this, &Tab::onStatusChanged,      connType);
  connect(&job, &Jobs::Job::progressChanged, this, &Tab::onJobProgressChanged, connType);
  connect(&job, &Jobs::Job::lineRead,        this, &Tab::onLineRead,           connType);
}

void
Tab::disconnectFromJob(Jobs::Job const &job) {
  auto p = p_func();

  if (p->m_currentlyConnectedJob == &job) {
    p->m_currentlyConnectedJob = nullptr;
    p->m_id.reset();
  }

  disconnect(&job, &Jobs::Job::statusChanged,   this, &Tab::onStatusChanged);
  disconnect(&job, &Jobs::Job::progressChanged, this, &Tab::onJobProgressChanged);
  disconnect(&job, &Jobs::Job::lineRead,        this, &Tab::onLineRead);
}

uint64_t
Tab::queueProgress()
  const {
  return p_func()->m_queueProgress;
}

void
Tab::onAbort() {
  auto p = p_func();

  if (!p->m_id)
    return;

  if (   Util::Settings::get().m_warnBeforeAbortingJobs
      && (Util::MessageBox::question(this)
            ->title(QY("Abort running job"))
            .text(QY("Do you really want to abort this job?"))
            .buttonLabel(QMessageBox::Yes, QY("&Abort job"))
            .buttonLabel(QMessageBox::No,  QY("Cancel"))
            .exec() == QMessageBox::No))
    return;

  MainWindow::jobTool()->model()->withJob(*p->m_id, [](Jobs::Job &job) { job.abort(); });
}

void
Tab::onStatusChanged(uint64_t id,
                     mtx::gui::Jobs::Job::Status,
                     mtx::gui::Jobs::Job::Status newStatus) {
  auto p = p_func();

  if (QObject::sender() != p->m_currentlyConnectedJob)
    return;

  auto job = MainWindow::jobTool()->model()->fromId(id);
  if (!job) {
    p->ui->abortButton->setEnabled(false);
    p->m_saveOutputAction->setEnabled(false);
    MainWindow::watchJobTool()->enableMenuActions();

    p->m_id.reset();
    p->m_currentJobProgress = 0;
    p->m_currentJobProgress = Jobs::Job::Aborted;
    updateRemainingTime();

    return;
  }

  job->action([this, p, job, newStatus]() {
    p->m_currentJobStatus = job->status();

    p->ui->abortButton->setEnabled(Jobs::Job::Running == p->m_currentJobStatus);
    p->m_saveOutputAction->setEnabled(true);
    p->ui->status->setText(Jobs::Job::displayableStatus(p->m_currentJobStatus));
    MainWindow::watchJobTool()->enableMenuActions();

    // Check for the signalled status, not the current one, in order to
    // detect a change from "not running" to "running" only once, no
    // matter which order the Q_SIGNALS arrive in.
    if (Jobs::Job::Running == newStatus)
      setInitialDisplay(*job);

    else if (mtx::included_in(p->m_currentJobStatus, Jobs::Job::DoneOk, Jobs::Job::DoneWarnings, Jobs::Job::Failed, Jobs::Job::Aborted))
      p->ui->finishedAt->setText(Util::displayableDate(job->dateFinished()));
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
    label->setText(Q(mtx::string::create_minutes_seconds_time_string(remainingDuration / 1000)));
  }
}

void
Tab::updateRemainingTime() {
  auto p = p_func();

  if ((Jobs::Job::Running != p->m_currentJobStatus) || !p->m_currentJobProgress)
    p->ui->remainingTimeCurrentJob->setText(Q("–"));

  else
    updateOneRemainingTimeLabel(p->ui->remainingTimeCurrentJob, p->m_currentJobStartTime, p->m_currentJobProgress);

  auto model = MainWindow::jobTool()->model();
  if (!model->isRunning())
    p->ui->remainingTimeQueue->setText(Q("–"));

  else
    updateOneRemainingTimeLabel(p->ui->remainingTimeQueue, model->queueStartTime(), p->m_queueProgress);
}

void
Tab::onQueueProgressChanged(int,
                            int totalProgress) {
  p_func()->m_queueProgress = totalProgress;
  updateRemainingTime();
}

void
Tab::onJobProgressChanged(uint64_t,
                          unsigned int progress) {
  auto p = p_func();

  if (QObject::sender() != p->m_currentlyConnectedJob)
    return;

  p->ui->progressBar->setValue(progress);
  p->m_currentJobProgress = progress;
  updateRemainingTime();
}

void
Tab::onLineRead(QString const &line,
                Jobs::Job::LineType type) {
  auto p = p_func();

  if ((QObject::sender() != p->m_currentlyConnectedJob) || line.isEmpty())
    return;

  auto &storage = Jobs::Job::InfoLine    == type ? p->ui->output
                : Jobs::Job::WarningLine == type ? p->ui->warnings
                :                                  p->ui->errors;

  auto prefix   = Jobs::Job::InfoLine    == type ? Q("")
                : Jobs::Job::WarningLine == type ? Q("%1 ").arg(QY("Warning:"))
                :                                  Q("%1 ").arg(QY("Error:"));

  if (mtx::included_in(type, Jobs::Job::WarningLine, Jobs::Job::ErrorLine)) {
    p->ui->acknowledgeWarningsAndErrorsButton->setEnabled(true);

    if (isCurrentJobTab() && Util::Settings::get().m_showOutputOfAllJobs && !p->m_currentJobLineTypeSeen[type]) {
      p->m_currentJobLineTypeSeen[type] = true;
      auto dateStarted                  = Util::displayableDate(p->m_currentJobStartTime);
      auto separator                    = Jobs::Job::WarningLine == type ? QY("--- Warnings emitted by job '%1' started on %2 ---").arg(p->m_currentJobDescription).arg(dateStarted)
                                        :                                  QY("--- Errors emitted by job '%1' started on %2 ---"  ).arg(p->m_currentJobDescription).arg(dateStarted);

      storage->appendPlainText(separator);
    }
  }

  p->m_fullOutput << Q("%1%2").arg(prefix).arg(line);
  storage->appendPlainText(line);

}

void
Tab::setInitialDisplay(Jobs::Job const &job) {
  auto p                     = p_func();
  auto dateStarted           = Util::displayableDate(job.dateStarted());
  p->m_currentJobDescription = job.description();

  if (isCurrentJobTab() && Util::Settings::get().m_showOutputOfAllJobs) {
    auto outputOfJobLine = QY("--- Output of job '%1' started on %2 ---").arg(p->m_currentJobDescription).arg(dateStarted);
    p->m_fullOutput << outputOfJobLine << job.fullOutput();

    p->ui->output->appendPlainText(outputOfJobLine);

  } else {
    p->m_fullOutput = job.fullOutput();

    p->ui->output  ->setPlainText(!job.output().isEmpty()   ? Q("%1\n").arg(job.output().join("\n"))   : Q(""));
    p->ui->warnings->setPlainText(!job.warnings().isEmpty() ? Q("%1\n").arg(job.warnings().join("\n")) : Q(""));
    p->ui->errors  ->setPlainText(!job.errors().isEmpty()   ? Q("%1\n").arg(job.errors().join("\n"))   : Q(""));
  }

  p->m_currentJobLineTypeSeen.clear();

  p->m_currentJobStatus    = job.status();
  p->m_currentJobProgress  = job.progress();
  p->m_currentJobStartTime = job.dateStarted();
  p->m_queueProgress       = MainWindow::watchCurrentJobTab()->queueProgress();

  p->ui->description->setText(p->m_currentJobDescription);
  p->ui->status->setText(Jobs::Job::displayableStatus(job.status()));
  p->ui->progressBar->setValue(job.progress());

  p->ui->startedAt ->setText(job.dateStarted() .isValid() ? Util::displayableDate(job.dateStarted())  : QY("Not started yet"));
  p->ui->finishedAt->setText(job.dateFinished().isValid() ? Util::displayableDate(job.dateFinished()) : QY("Not finished yet"));

  p->ui->abortButton->setEnabled(Jobs::Job::Running == job.status());
  p->m_saveOutputAction->setEnabled(!mtx::included_in(job.status(), Jobs::Job::PendingManual, Jobs::Job::PendingAuto, Jobs::Job::Disabled));

  p->ui->acknowledgeWarningsAndErrorsButton->setEnabled(job.numUnacknowledgedWarnings() || job.numUnacknowledgedErrors());

  updateRemainingTime();
}

void
Tab::disableButtonIfAllWarningsAndErrorsButtonAcknowledged(int numWarnings,
                                                           int numErrors) {
  auto p = p_func();

  if (!numWarnings && !numErrors)
    p->ui->acknowledgeWarningsAndErrorsButton->setEnabled(false);
}

void
Tab::onSaveOutput() {
  auto p        = p_func();
  auto &cfg     = Util::Settings::get();
  auto txtName  = Util::replaceInvalidFileNameCharacters(p->m_currentJobDescription) + Q(".txt");
  auto fileName = Util::getSaveFileName(this, QY("Save job output"), cfg.lastOpenDirPath(), txtName, QY("Text files") + Q(" (*.txt);;") + QY("All files") + Q(" (*)"), Q("txt"));

  if (fileName.isEmpty())
    return;

  QFile out{fileName};
  if (out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    out.write(Q("%1\n").arg(p->m_fullOutput.join(Q("\n"))).toUtf8());
    out.flush();
    out.close();
  }

  cfg.m_lastOpenDir.setPath(QFileInfo{fileName}.path());
  cfg.save();
}

std::optional<uint64_t>
Tab::id()
  const {
  return p_func()->m_id;
}

bool
Tab::isSaveOutputEnabled()
  const {
  return p_func()->m_saveOutputAction->isEnabled();
}

bool
Tab::isCurrentJobTab()
  const {
  return p_func()->m_forCurrentJob;
}

void
Tab::acknowledgeWarningsAndErrors() {
  auto p = p_func();

  if (p->m_id)
    MainWindow::jobTool()->acknowledgeWarningsAndErrors(*p->m_id);
  p->ui->acknowledgeWarningsAndErrorsButton->setEnabled(false);
}

void
Tab::clearOutput() {
  auto p = p_func();

  p->ui->output->clear();
  p->ui->warnings->clear();
  p->ui->errors->clear();
  p->ui->acknowledgeWarningsAndErrorsButton->setEnabled(false);
  p->m_fullOutput.clear();

  if (MainWindow::jobTool()->model()->isRunning())
    return;

  p->m_currentJobDescription.clear();
  p->m_id.reset();

  p->ui->progressBar->reset();
  p->ui->status->setText(QY("No job started yet"));
  p->ui->description->setText(QY("No job has been started yet."));
  p->ui->startedAt->setText(QY("Not started yet"));
  p->ui->finishedAt->setText(QY("Not finished yet"));
  p->ui->remainingTimeCurrentJob->setText(Q("–"));
  p->ui->remainingTimeQueue->setText(Q("–"));

  Q_EMIT watchCurrentJobTabCleared();
}

void
Tab::openFolder() {
  auto p = p_func();

  if (p->m_id)
    MainWindow::jobTool()->model()->withJob(*p->m_id, [](Jobs::Job &job) { job.openOutputFolder(); });
}

void
Tab::enableMoreActionsActions() {
  auto p      = p_func();
  auto hasJob = !!p->m_id;

  p->m_openFolderAction->setEnabled(hasJob);
  p->m_saveOutputAction->setEnabled(!p->ui->output->toPlainText().isEmpty() || !p->ui->warnings->toPlainText().isEmpty() || !p->ui->errors->toPlainText().isEmpty());
}

void
Tab::setupWhenFinishedActions() {
  auto p = p_func();

  p->m_whenFinished->clear();

  auto afterCurrentJobMenu                = new QMenu{QY("Execute action after next &job completion")};
  auto afterJobQueueMenu                  = new QMenu{QY("Execute action when the &queue completes")};
  auto editRunProgramConfigurationsAction = new QAction{p->m_whenFinished};
  auto menus                              = QVector<QMenu *>{} << afterCurrentJobMenu << afterJobQueueMenu;
  auto &programRunner                     = App::programRunner();

  editRunProgramConfigurationsAction->setText(QY("&Edit available actions to execute"));

  p->m_whenFinished->addMenu(afterCurrentJobMenu);
  p->m_whenFinished->addMenu(afterJobQueueMenu);
  p->m_whenFinished->addSeparator();
  p->m_whenFinished->addAction(editRunProgramConfigurationsAction);

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

}
