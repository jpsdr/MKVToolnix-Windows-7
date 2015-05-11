#include "common/common_pch.h"

#include "common/qt.h"
#include "mkvtoolnix-gui/forms/jobs/tool.h"
#include "mkvtoolnix-gui/forms/main_window/main_window.h"
#include "mkvtoolnix-gui/jobs/mux_job.h"
#include "mkvtoolnix-gui/jobs/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/merge/mux_config.h"
#include "mkvtoolnix-gui/util/util.h"
#include "mkvtoolnix-gui/watch_jobs/tab.h"

#include <QList>
#include <QMessageBox>
#include <QString>

namespace mtx { namespace gui { namespace Jobs {

Tool::Tool(QWidget *parent)
  : ToolBase{parent}
  , ui{new Ui::Tool}
  , m_model{new Model{this}}
  , m_startAction{new QAction{this}}
  , m_removeAction{new QAction{this}}
  , m_removeDoneAction{new QAction{this}}
  , m_removeDoneOkAction{new QAction{this}}
  , m_removeAllAction{new QAction{this}}
{
  // Setup UI controls.
  ui->setupUi(this);

  setupUiControls();
}

Tool::~Tool() {
}

void
Tool::loadAndStart() {
  QSettings reg;
  m_model->loadJobs(reg);

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
  ui->jobs->setModel(m_model);

  connect(m_startAction,        SIGNAL(triggered()), this, SLOT(onStart()));
  connect(m_removeAction,       SIGNAL(triggered()), this, SLOT(onRemove()));
  connect(m_removeDoneAction,   SIGNAL(triggered()), this, SLOT(onRemoveDone()));
  connect(m_removeDoneOkAction, SIGNAL(triggered()), this, SLOT(onRemoveDoneOk()));
  connect(m_removeAllAction,    SIGNAL(triggered()), this, SLOT(onRemoveAll()));
}

void
Tool::onContextMenu(QPoint pos) {
  bool hasJobs      = m_model->hasJobs();
  bool hasSelection = !m_model->selectedJobs(ui->jobs).isEmpty();

  m_startAction->setEnabled(hasSelection);
  m_removeAction->setEnabled(hasSelection);
  m_removeDoneAction->setEnabled(hasJobs);
  m_removeDoneOkAction->setEnabled(hasJobs);
  m_removeAllAction->setEnabled(hasJobs);

  QMenu menu{this};

  menu.addSeparator();
  menu.addAction(m_startAction);
  menu.addSeparator();
  menu.addAction(m_removeAction);
  menu.addAction(m_removeDoneAction);
  menu.addAction(m_removeDoneOkAction);
  menu.addAction(m_removeAllAction);

  menu.exec(ui->jobs->viewport()->mapToGlobal(pos));
}

void
Tool::onStart() {
  auto jobs = m_model->selectedJobs(ui->jobs);
  for (auto const &job : jobs)
    job->setPendingAuto();

  m_model->startNextAutoJob();
}

void
Tool::onRemove() {
  auto idsToRemove  = QMap<uint64_t, bool>{};
  auto selectedJobs = m_model->selectedJobs(ui->jobs);

  for (auto const &job : selectedJobs)
    idsToRemove[job->m_id] = true;

  m_model->removeJobsIf([&](Job const &job) { return idsToRemove[job.m_id]; });
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
  m_model->removeJobsIf([this](Job const &) { return true; });
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
  m_removeAction->setText(QY("&Remove selected jobs"));
  m_removeDoneAction->setText(QY("Remove &completed jobs"));
  m_removeDoneOkAction->setText(QY("Remove &successfully completed jobs"));
  m_removeAllAction->setText(QY("Remove a&ll jobs"));

  setupToolTips();
}

void
Tool::setupToolTips() {
  Util::setToolTip(ui->jobs, QY("Right-click for actions for jobs"));
}

void
Tool::toolShown() {
  MainWindow::get()->showTheseMenusOnly({ MainWindow::getUi()->menuMerge });
}

}}}
