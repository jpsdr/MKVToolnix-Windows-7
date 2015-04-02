#include "common/common_pch.h"

#include "common/qt.h"
#include "mkvtoolnix-gui/forms/watch_jobs/tab.h"
#include "mkvtoolnix-gui/jobs/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/util/util.h"
#include "mkvtoolnix-gui/watch_jobs/tab.h"

namespace mtx { namespace gui { namespace WatchJobs {

Tab::Tab(QWidget *parent)
  : QWidget{parent}
  , ui{new Ui::Tab}
{
  // Setup UI controls.
  ui->setupUi(this);
}

Tab::~Tab() {
}

void
Tab::connectToJob(mtx::gui::Jobs::Job const &job) {
  connect(&job, SIGNAL(statusChanged(uint64_t,mtx::gui::Jobs::Job::Status)),    this, SLOT(onStatusChanged(uint64_t,mtx::gui::Jobs::Job::Status)));
  connect(&job, SIGNAL(progressChanged(uint64_t,unsigned int)),                 this, SLOT(onProgressChanged(uint64_t,unsigned int)));
  connect(&job, SIGNAL(lineRead(const QString&,mtx::gui::Jobs::Job::LineType)), this, SLOT(onLineRead(const QString&,mtx::gui::Jobs::Job::LineType)));
}

void
Tab::onStatusChanged(uint64_t id,
                     mtx::gui::Jobs::Job::Status status) {
  auto job = MainWindow::getJobTool()->getModel()->fromId(id);
  if (!job)
    return;

  ui->status->setText(mtx::gui::Jobs::Job::displayableStatus(status));

  if (mtx::gui::Jobs::Job::Running == status)
    setInitialDisplay(*job);

  else if ((mtx::gui::Jobs::Job::DoneOk == status) || (mtx::gui::Jobs::Job::DoneWarnings == status) || (mtx::gui::Jobs::Job::Failed == status) || (mtx::gui::Jobs::Job::Aborted == status))
    ui->finishedAt->setText(Util::displayableDate(job->m_dateFinished));
}

void
Tab::onProgressChanged(uint64_t,
                       unsigned int progress) {
  ui->progressBar->setValue(progress);
}

void
Tab::onLineRead(QString const &line,
                mtx::gui::Jobs::Job::LineType type) {
  auto &storage = mtx::gui::Jobs::Job::InfoLine    == type ? ui->output
                : mtx::gui::Jobs::Job::WarningLine == type ? ui->warnings
                :                                            ui->errors;

  storage->appendPlainText(line);
}

void
Tab::setInitialDisplay(mtx::gui::Jobs::Job const &job) {
  ui->description->setText(job.m_description);
  ui->progressBar->setValue(job.m_progress);

  ui->output  ->setPlainText(job.m_output.isEmpty()   ? Q("%1\n").arg(job.m_output.join("\n"))   : Q(""));
  ui->warnings->setPlainText(job.m_warnings.isEmpty() ? Q("%1\n").arg(job.m_warnings.join("\n")) : Q(""));
  ui->errors  ->setPlainText(job.m_errors.isEmpty()   ? Q("%1\n").arg(job.m_errors.join("\n"))   : Q(""));

  ui->startedAt ->setText(job.m_dateStarted .isValid() ? Util::displayableDate(job.m_dateStarted)  : QY("not started yet"));
  ui->finishedAt->setText(job.m_dateFinished.isValid() ? Util::displayableDate(job.m_dateFinished) : QY("not finished yet"));
}

}}}
