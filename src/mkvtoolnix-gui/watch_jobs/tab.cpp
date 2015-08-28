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
#include "mkvtoolnix-gui/forms/watch_jobs/tab.h"
#include "mkvtoolnix-gui/jobs/mux_job.h"
#include "mkvtoolnix-gui/jobs/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/util/message_box.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/string.h"
#include "mkvtoolnix-gui/util/widget.h"
#include "mkvtoolnix-gui/watch_jobs/tab.h"
#include "mkvtoolnix-gui/watch_jobs/tool.h"

namespace mtx { namespace gui { namespace WatchJobs {

using namespace mtx::gui;

Tab::Tab(QWidget *parent)
  : QWidget{parent}
  , ui{new Ui::Tab}
  , m_id{std::numeric_limits<uint64_t>::max()}
  , m_currentJobProgress{}
  , m_queueProgress{}
  , m_currentJobStatus{Jobs::Job::PendingManual}
  , m_currentlyConnectedJob{}
  , m_saveOutputAction{new QAction{this}}
  , m_clearOutputAction{new QAction{this}}
  , m_openFolderAction{new QAction{this}}
{
  // Setup UI controls.
  ui->setupUi(this);

  setupUi();
}

Tab::~Tab() {
}

void
Tab::setupUi() {
  m_saveOutputAction->setEnabled(false);

  Util::preventScrollingWithoutFocus(this);

  auto &cfg = Util::Settings::get();
  for (auto const &splitter : findChildren<QSplitter *>())
    cfg.handleSplitterSizes(splitter);

  retranslateUi();

  auto model = MainWindow::jobTool()->model();

  connect(ui->abortButton,                        &QPushButton::clicked,            this, &Tab::onAbort);
  connect(ui->acknowledgeWarningsAndErrorsButton, &QPushButton::clicked,            this, &Tab::acknowledgeWarningsAndErrors);
  connect(ui->moreActionsButton,                  &QPushButton::clicked,            this, &Tab::showMoreActionsMenu);
  connect(model,                                  &Jobs::Model::progressChanged,    this, &Tab::onQueueProgressChanged);
  connect(model,                                  &Jobs::Model::queueStatusChanged, this, &Tab::updateRemainingTime);
  connect(m_saveOutputAction,                     &QAction::triggered,              this, &Tab::onSaveOutput);
  connect(m_clearOutputAction,                    &QAction::triggered,              this, &Tab::clearOutput);
  connect(m_openFolderAction,                     &QAction::triggered,              this, &Tab::openFolder);
}

void
Tab::retranslateUi() {
  ui->retranslateUi(this);

  ui->description->setText(m_currentJobDescription.isEmpty() ? QY("No job has been started yet.") : m_currentJobDescription);
  m_saveOutputAction->setText(QY("&Save output"));
  m_clearOutputAction->setText(QY("&Clear output and reset progress"));
  m_openFolderAction->setText(QY("&Open folder"));

#if (QT_VERSION >= QT_VERSION_CHECK(5, 3, 0))
  ui->output->setPlaceholderText(QY("no output yet"));
  ui->warnings->setPlaceholderText(QY("no warnings yet"));
  ui->errors->setPlaceholderText(QY("no errors yet"));
#endif
}

void
Tab::connectToJob(Jobs::Job const &job) {
  m_currentlyConnectedJob = &job;
  m_id                    = job.id();
  auto connType           = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(&job, &Jobs::Job::statusChanged,   this, &Tab::onStatusChanged,      connType);
  connect(&job, &Jobs::Job::progressChanged, this, &Tab::onJobProgressChanged, connType);
  connect(&job, &Jobs::Job::lineRead,        this, &Tab::onLineRead,           connType);
}

void
Tab::disconnectFromJob(Jobs::Job const &job) {
  if (m_currentlyConnectedJob == &job) {
    m_currentlyConnectedJob = nullptr;
    m_id                    = std::numeric_limits<uint64_t>::max();
  }

  disconnect(&job, &Jobs::Job::statusChanged,   this, &Tab::onStatusChanged);
  disconnect(&job, &Jobs::Job::progressChanged, this, &Tab::onJobProgressChanged);
  disconnect(&job, &Jobs::Job::lineRead,        this, &Tab::onLineRead);
}

void
Tab::onAbort() {
  if (std::numeric_limits<uint64_t>::max() == m_id)
    return;

  if (   Util::Settings::get().m_warnBeforeAbortingJobs
      && (Util::MessageBox::question(this).title(QY("Abort running jobs")).text(QY("Do you really want to abort this job?")).exec() == QMessageBox::No))
    return;

  MainWindow::jobTool()->model()->withJob(m_id, [](Jobs::Job &job) { job.abort(); });
}

void
Tab::onStatusChanged(uint64_t id,
                     mtx::gui::Jobs::Job::Status,
                     mtx::gui::Jobs::Job::Status newStatus) {
  if (QObject::sender() != m_currentlyConnectedJob)
    return;

  auto job = MainWindow::jobTool()->model()->fromId(id);
  if (!job) {
    ui->abortButton->setEnabled(false);
    m_saveOutputAction->setEnabled(false);
    MainWindow::watchJobTool()->enableMenuActions();

    m_id                 = std::numeric_limits<uint64_t>::max();
    m_currentJobProgress = 0;
    m_currentJobProgress = Jobs::Job::Aborted;
    updateRemainingTime();

    return;
  }

  job->action([&]() {
    m_currentJobStatus = job->status();

    ui->abortButton->setEnabled(Jobs::Job::Running == m_currentJobStatus);
    m_saveOutputAction->setEnabled(true);
    ui->status->setText(Jobs::Job::displayableStatus(m_currentJobStatus));
    MainWindow::watchJobTool()->enableMenuActions();

    // Check for the signalled status, not the current one, in order to
    // detect a change from »not running« to »running« only once, no
    // matter which order the signals arrive in.
    if (Jobs::Job::Running == newStatus)
      setInitialDisplay(*job);

    else if (mtx::included_in(m_currentJobStatus, Jobs::Job::DoneOk, Jobs::Job::DoneWarnings, Jobs::Job::Failed, Jobs::Job::Aborted))
      ui->finishedAt->setText(Util::displayableDate(job->dateFinished()));
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
  if ((Jobs::Job::Running != m_currentJobStatus) || !m_currentJobProgress)
    ui->remainingTimeCurrentJob->setText(Q("–"));

  else
    updateOneRemainingTimeLabel(ui->remainingTimeCurrentJob, m_currentJobStartTime, m_currentJobProgress);

  auto model = MainWindow::jobTool()->model();
  if (!model->isRunning())
    ui->remainingTimeQueue->setText(Q("–"));

  else
    updateOneRemainingTimeLabel(ui->remainingTimeQueue, model->queueStartTime(), m_queueProgress);
}

void
Tab::onQueueProgressChanged(int,
                            int totalProgress) {
  m_queueProgress = totalProgress;
  updateRemainingTime();
}

void
Tab::onJobProgressChanged(uint64_t,
                          unsigned int progress) {
  if (QObject::sender() != m_currentlyConnectedJob)
    return;

  ui->progressBar->setValue(progress);
  m_currentJobProgress = progress;
  updateRemainingTime();
}

void
Tab::onLineRead(QString const &line,
                Jobs::Job::LineType type) {
  if ((QObject::sender() != m_currentlyConnectedJob) || line.isEmpty())
    return;

  auto &storage = Jobs::Job::InfoLine    == type ? ui->output
                : Jobs::Job::WarningLine == type ? ui->warnings
                :                                  ui->errors;

  auto prefix   = Jobs::Job::InfoLine    == type ? Q("")
                : Jobs::Job::WarningLine == type ? Q("%1 ").arg(QY("Warning:"))
                :                                  Q("%1 ").arg(QY("Error:"));

  m_fullOutput << Q("%1%2").arg(prefix).arg(line);
  storage->appendPlainText(line);

  if (mtx::included_in(type, Jobs::Job::WarningLine, Jobs::Job::ErrorLine))
    ui->acknowledgeWarningsAndErrorsButton->setEnabled(true);
}

void
Tab::setInitialDisplay(Jobs::Job const &job) {
  auto dateStarted        = Util::displayableDate(job.dateStarted());
  m_currentJobDescription = job.description();

  if (isCurrentJobTab() && Util::Settings::get().m_showOutputOfAllJobs) {
    auto outputOfJobLine = QY("--- Output of job '%1' started on %2 ---").arg(m_currentJobDescription).arg(dateStarted);
    m_fullOutput << outputOfJobLine << job.fullOutput();

    ui->output  ->appendPlainText(outputOfJobLine);
    ui->warnings->appendPlainText(QY("--- Warnings emitted by job '%1' started on %2 ---").arg(m_currentJobDescription).arg(dateStarted));
    ui->errors  ->appendPlainText(QY("--- Errors emitted by job '%1' started on %2 ---").arg(m_currentJobDescription).arg(dateStarted));

  } else {
    m_fullOutput = job.fullOutput();

    ui->output  ->setPlainText(!job.output().isEmpty()   ? Q("%1\n").arg(job.output().join("\n"))   : Q(""));
    ui->warnings->setPlainText(!job.warnings().isEmpty() ? Q("%1\n").arg(job.warnings().join("\n")) : Q(""));
    ui->errors  ->setPlainText(!job.errors().isEmpty()   ? Q("%1\n").arg(job.errors().join("\n"))   : Q(""));
  }

  m_currentJobStatus    = job.status();
  m_currentJobProgress  = job.progress();
  m_currentJobStartTime = job.dateStarted();
  m_queueProgress       = MainWindow::watchCurrentJobTab()->m_queueProgress;

  ui->description->setText(m_currentJobDescription);
  ui->status->setText(Jobs::Job::displayableStatus(job.status()));
  ui->progressBar->setValue(job.progress());

  ui->startedAt ->setText(job.dateStarted() .isValid() ? Util::displayableDate(job.dateStarted())  : QY("not started yet"));
  ui->finishedAt->setText(job.dateFinished().isValid() ? Util::displayableDate(job.dateFinished()) : QY("not finished yet"));

  ui->abortButton->setEnabled(Jobs::Job::Running == job.status());
  m_saveOutputAction->setEnabled(!mtx::included_in(job.status(), Jobs::Job::PendingManual, Jobs::Job::PendingAuto, Jobs::Job::Disabled));

  ui->acknowledgeWarningsAndErrorsButton->setEnabled(job.numUnacknowledgedWarnings() || job.numUnacknowledgedErrors());

  updateRemainingTime();
}

void
Tab::onSaveOutput() {
  auto &cfg = Util::Settings::get();

  auto fileName = QFileDialog::getSaveFileName(this, QY("Save job output"), cfg.m_lastOpenDir.path(), QY("Text files") + Q(" (*.txt);;") + QY("All files") + Q(" (*)"));
  if (fileName.isEmpty())
    return;

  QFile out{fileName};
  if (out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    out.write(Q("%1\n").arg(m_fullOutput.join(Q("\n"))).toUtf8());
    out.close();
  }

  cfg.m_lastOpenDir = QFileInfo{fileName}.path();
  cfg.save();
}

uint64_t
Tab::id()
  const {
  return m_id;
}

bool
Tab::isSaveOutputEnabled()
  const {
  return m_saveOutputAction->isEnabled();
}

bool
Tab::isCurrentJobTab()
  const {
  return this == MainWindow::get()->watchCurrentJobTab();
}

void
Tab::acknowledgeWarningsAndErrors() {
  MainWindow::jobTool()->acknowledgeWarningsAndErrors(m_id);
  ui->acknowledgeWarningsAndErrorsButton->setEnabled(false);
}

void
Tab::clearOutput() {
  ui->output->clear();
  ui->warnings->clear();
  ui->errors->clear();

  if (MainWindow::jobTool()->model()->isRunning())
    return;

  m_currentJobDescription.clear();

  ui->progressBar->reset();
  ui->status->setText(QY("no job started yet"));
  ui->description->setText(QY("No job has been started yet."));
  ui->startedAt->setText(QY("not started yet"));
  ui->finishedAt->setText(QY("not finished yet"));
  ui->remainingTimeCurrentJob->setText(Q("–"));
  ui->remainingTimeQueue->setText(Q("–"));

  emit watchCurrentJobTabCleared();
}

void
Tab::openFolder() {
  MainWindow::jobTool()->model()->withJob(m_id, [](Jobs::Job &job) { job.openOutputFolder(); });
}

void
Tab::showMoreActionsMenu() {
  QMenu menu{this};

  auto hasJob = std::numeric_limits<uint64_t>::max() != m_id;
  m_openFolderAction->setEnabled(hasJob);
  m_clearOutputAction->setEnabled(true);

  menu.addAction(m_openFolderAction);
  menu.addSeparator();
  menu.addAction(m_saveOutputAction);

  if (isCurrentJobTab())
    menu.addAction(m_clearOutputAction);

  menu.exec(QCursor::pos());
}

}}}
