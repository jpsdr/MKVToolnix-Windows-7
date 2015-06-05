#include "common/common_pch.h"

#include <QFileDialog>
#include <QPushButton>
#include <QtGlobal>

#include "common/list_utils.h"
#include "common/qt.h"
#include "common/strings/formatting.h"
#include "mkvtoolnix-gui/forms/watch_jobs/tab.h"
#include "mkvtoolnix-gui/jobs/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/util.h"
#include "mkvtoolnix-gui/watch_jobs/tab.h"
#include "mkvtoolnix-gui/watch_jobs/tool.h"

namespace mtx { namespace gui { namespace WatchJobs {

using namespace mtx::gui;

Tab::Tab(QWidget *parent)
  : QWidget{parent}
  , ui{new Ui::Tab}
  , m_currentJobProgress{}
  , m_queueProgress{}
  , m_currentJobStatus{Jobs::Job::PendingManual}
  , m_currentlyConnectedJob{}
{
  // Setup UI controls.
  ui->setupUi(this);
}

Tab::~Tab() {
}

void
Tab::retranslateUi() {
  ui->retranslateUi(this);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 3, 0))
  ui->output->setPlaceholderText(QY("no output yet"));
  ui->warnings->setPlaceholderText(QY("no warnings yet"));
  ui->errors->setPlaceholderText(QY("no errors yet"));
#endif
}

void
Tab::connectToJob(Jobs::Job const &job) {
  m_currentlyConnectedJob = &job;
  m_id                    = job.m_id;
  auto connType           = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);
  auto model              = MainWindow::jobTool()->model();

  ui->abortButton->disconnect();

  connect(&job,                                   &Jobs::Job::statusChanged,     this,    &Tab::onStatusChanged,              connType);
  connect(&job,                                   &Jobs::Job::progressChanged,   this,    &Tab::onJobProgressChanged,         connType);
  connect(&job,                                   &Jobs::Job::lineRead,          this,    &Tab::onLineRead,                   connType);
  connect(ui->abortButton,                        &QPushButton::clicked,         &job,    &Jobs::Job::abort,                  connType);
  connect(ui->saveOutputButton,                   &QPushButton::clicked,         this,    &Tab::onSaveOutput,                 connType);
  connect(ui->acknowledgeWarningsAndErrorsButton, &QPushButton::clicked,         this,    &Tab::acknowledgeWarningsAndErrors, connType);
  connect(model,                                  &Jobs::Model::progressChanged, this,    &Tab::onQueueProgressChanged,       connType);
  connect(model,                                  &Jobs::Model::queueStarted,    this,    &Tab::updateRemainingTime,          connType);
  connect(model,                                  &Jobs::Model::queueStopped,    this,    &Tab::updateRemainingTime,          connType);
}

void
Tab::onStatusChanged(uint64_t id) {
  if (QObject::sender() != m_currentlyConnectedJob)
    return;

  auto job = MainWindow::jobTool()->model()->fromId(id);
  if (!job) {
    ui->abortButton->setEnabled(false);
    ui->saveOutputButton->setEnabled(false);
    MainWindow::watchJobTool()->enableMenuActions();

    m_currentJobProgress = 0;
    m_currentJobProgress = Jobs::Job::Aborted;
    updateRemainingTime();

    return;
  }

  QMutexLocker locker{&job->m_mutex};
  m_currentJobStatus = job->m_status;

  ui->abortButton->setEnabled(Jobs::Job::Running == m_currentJobStatus);
  ui->saveOutputButton->setEnabled(true);
  ui->status->setText(Jobs::Job::displayableStatus(m_currentJobStatus));
  MainWindow::watchJobTool()->enableMenuActions();

  if (Jobs::Job::Running == m_currentJobStatus) {
    setInitialDisplay(*job);
    m_currentJobStartTime = job->m_dateStarted;

  } else if (mtx::included_in(m_currentJobStatus, Jobs::Job::DoneOk, Jobs::Job::DoneWarnings, Jobs::Job::Failed, Jobs::Job::Aborted))
    ui->finishedAt->setText(Util::displayableDate(job->m_dateFinished));

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
  if (QObject::sender() != m_currentlyConnectedJob)
    return;

  auto &storage = Jobs::Job::InfoLine    == type ? ui->output
                : Jobs::Job::WarningLine == type ? ui->warnings
                :                                  ui->errors;

  storage->appendPlainText(line);
  m_fullOutput << line;

  if (mtx::included_in(type, Jobs::Job::WarningLine, Jobs::Job::ErrorLine))
    ui->acknowledgeWarningsAndErrorsButton->setEnabled(true);
}

void
Tab::setInitialDisplay(Jobs::Job const &job) {
  ui->description->setText(job.m_description);
  ui->status->setText(Jobs::Job::displayableStatus(job.m_status));
  ui->progressBar->setValue(job.m_progress);

  ui->output  ->setPlainText(!job.m_output.isEmpty()   ? Q("%1\n").arg(job.m_output.join("\n"))   : Q(""));
  ui->warnings->setPlainText(!job.m_warnings.isEmpty() ? Q("%1\n").arg(job.m_warnings.join("\n")) : Q(""));
  ui->errors  ->setPlainText(!job.m_errors.isEmpty()   ? Q("%1\n").arg(job.m_errors.join("\n"))   : Q(""));

  ui->startedAt ->setText(job.m_dateStarted .isValid() ? Util::displayableDate(job.m_dateStarted)  : QY("not started yet"));
  ui->finishedAt->setText(job.m_dateFinished.isValid() ? Util::displayableDate(job.m_dateFinished) : QY("not finished yet"));

  m_fullOutput = job.m_fullOutput;

  ui->abortButton->setEnabled(Jobs::Job::Running == job.m_status);
  ui->saveOutputButton->setEnabled(!mtx::included_in(job.m_status, Jobs::Job::PendingManual, Jobs::Job::PendingAuto, Jobs::Job::Disabled));

  ui->acknowledgeWarningsAndErrorsButton->setEnabled(job.numUnacknowledgedWarnings() || job.numUnacknowledgedErrors());

  updateRemainingTime();
}

void
Tab::onSaveOutput() {
  auto &cfg = Util::Settings::get();

  auto fileName = QFileDialog::getSaveFileName(this, QY("Save job output"), cfg.m_lastConfigDir.path(), QY("Text files") + Q(" (*.txt);;") + QY("All files") + Q(" (*)"));
  if (fileName.isEmpty())
    return;

  QFile out{fileName};
  if (out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    out.write(Q("%1\n").arg(m_fullOutput.join(Q("\n"))).toUtf8());
    out.close();
  }

  cfg.m_lastConfigDir = QFileInfo{fileName}.path();
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
  return ui->saveOutputButton->isEnabled();
}

void
Tab::acknowledgeWarningsAndErrors() {
  MainWindow::jobTool()->acknowledgeWarningsAndErrors(m_id);
  ui->acknowledgeWarningsAndErrorsButton->setEnabled(false);
}

}}}
