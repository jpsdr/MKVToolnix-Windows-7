#include "common/common_pch.h"

#include <QPushButton>
#include <QtGlobal>

#include "common/qt.h"
#include "mkvtoolnix-gui/forms/watch_jobs/tab.h"
#include "mkvtoolnix-gui/jobs/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/util/util.h"
#include "mkvtoolnix-gui/watch_jobs/tab.h"

namespace mtx { namespace gui { namespace WatchJobs {

using namespace mtx::gui;

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
  connect(&job,            &Jobs::Job::statusChanged,   this, &Tab::onStatusChanged);
  connect(&job,            &Jobs::Job::progressChanged, this, &Tab::onProgressChanged);
  connect(&job,            &Jobs::Job::lineRead,        this, &Tab::onLineRead);
  connect(ui->abortButton, &QPushButton::clicked,       &job, &Jobs::Job::abort);
}

void
Tab::onStatusChanged(uint64_t id) {
  auto job = MainWindow::jobTool()->model()->fromId(id);
  if (!job) {
    ui->abortButton->setEnabled(false);
    return;
  }

  QMutexLocker locker{&job->m_mutex};
  auto status = job->m_status;

  ui->abortButton->setEnabled(Jobs::Job::Running == status);
  ui->status->setText(Jobs::Job::displayableStatus(status));

  if (Jobs::Job::Running == status)
    setInitialDisplay(*job);

  else if ((Jobs::Job::DoneOk == status) || (Jobs::Job::DoneWarnings == status) || (Jobs::Job::Failed == status) || (Jobs::Job::Aborted == status))
    ui->finishedAt->setText(Util::displayableDate(job->m_dateFinished));
}

void
Tab::onProgressChanged(uint64_t,
                       unsigned int progress) {
  ui->progressBar->setValue(progress);
}

void
Tab::onLineRead(QString const &line,
                Jobs::Job::LineType type) {
  auto &storage = Jobs::Job::InfoLine    == type ? ui->output
                : Jobs::Job::WarningLine == type ? ui->warnings
                :                                  ui->errors;

  storage->appendPlainText(line);
}

void
Tab::setInitialDisplay(Jobs::Job const &job) {
  ui->description->setText(job.m_description);
  ui->progressBar->setValue(job.m_progress);

  ui->output  ->setPlainText(!job.m_output.isEmpty()   ? Q("%1\n").arg(job.m_output.join("\n"))   : Q(""));
  ui->warnings->setPlainText(!job.m_warnings.isEmpty() ? Q("%1\n").arg(job.m_warnings.join("\n")) : Q(""));
  ui->errors  ->setPlainText(!job.m_errors.isEmpty()   ? Q("%1\n").arg(job.m_errors.join("\n"))   : Q(""));

  ui->startedAt ->setText(job.m_dateStarted .isValid() ? Util::displayableDate(job.m_dateStarted)  : QY("not started yet"));
  ui->finishedAt->setText(job.m_dateFinished.isValid() ? Util::displayableDate(job.m_dateFinished) : QY("not finished yet"));
}

void
Tab::onSaveOutput() {
  // TODO: Tab::onSaveOutput
}

}}}
