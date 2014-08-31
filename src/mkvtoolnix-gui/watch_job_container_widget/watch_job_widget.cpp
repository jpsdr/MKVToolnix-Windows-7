#include "common/common_pch.h"

#include "common/qt.h"
#include "mkvtoolnix-gui/forms/watch_job_widget.h"
#include "mkvtoolnix-gui/job_widget/job_widget.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/util/util.h"
#include "mkvtoolnix-gui/watch_job_container_widget/watch_job_widget.h"

WatchJobWidget::WatchJobWidget(QWidget *parent)
  : QWidget{parent}
  , ui{new Ui::WatchJobWidget}
{
  // Setup UI controls.
  ui->setupUi(this);
}

WatchJobWidget::~WatchJobWidget() {
}

void
WatchJobWidget::connectToJob(Job const &job) {
  connect(&job, SIGNAL(statusChanged(uint64_t,Job::Status)),    this, SLOT(onStatusChanged(uint64_t,Job::Status)));
  connect(&job, SIGNAL(progressChanged(uint64_t,unsigned int)), this, SLOT(onProgressChanged(uint64_t,unsigned int)));
  connect(&job, SIGNAL(lineRead(const QString&,Job::LineType)), this, SLOT(onLineRead(const QString&,Job::LineType)));
}

void
WatchJobWidget::onStatusChanged(uint64_t id,
                                Job::Status status) {
  auto job = MainWindow::getJobWidget()->getModel()->fromId(id);
  if (!job)
    return;

  ui->status->setText(Job::displayableStatus(status));

  if (Job::Running == status)
    setInitialDisplay(*job);

  else if ((Job::DoneOk == status) || (Job::DoneWarnings == status) || (Job::Failed == status) || (Job::Aborted == status))
    ui->finishedAt->setText(Util::displayableDate(job->m_dateFinished));
}

void
WatchJobWidget::onProgressChanged(uint64_t,
                                  unsigned int progress) {
  ui->progressBar->setValue(progress);
}

void
WatchJobWidget::onLineRead(QString const &line,
                           Job::LineType type) {
  auto &storage = Job::InfoLine    == type ? ui->output
                : Job::WarningLine == type ? ui->warnings
                :                            ui->errors;

  storage->appendPlainText(line);
}

void
WatchJobWidget::setInitialDisplay(Job const &job) {
  ui->description->setText(job.m_description);
  ui->progressBar->setValue(job.m_progress);

  ui->output  ->setPlainText(job.m_output.isEmpty()   ? Q("%1\n").arg(job.m_output.join("\n"))   : Q(""));
  ui->warnings->setPlainText(job.m_warnings.isEmpty() ? Q("%1\n").arg(job.m_warnings.join("\n")) : Q(""));
  ui->errors  ->setPlainText(job.m_errors.isEmpty()   ? Q("%1\n").arg(job.m_errors.join("\n"))   : Q(""));

  ui->startedAt ->setText(job.m_dateStarted .isValid() ? Util::displayableDate(job.m_dateStarted)  : QY("not started yet"));
  ui->finishedAt->setText(job.m_dateFinished.isValid() ? Util::displayableDate(job.m_dateFinished) : QY("not finished yet"));
}
