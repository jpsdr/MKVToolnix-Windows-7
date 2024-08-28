#include "common/common_pch.h"

#include <QAction>
#include <QMenu>
#include <QMouseEvent>
#include <QTimer>

#include "common/qt.h"
#include "mkvtoolnix-gui/forms/main_window/status_bar_progress_widget.h"
#include "mkvtoolnix-gui/jobs/tool.h"
#include "mkvtoolnix-gui/watch_jobs/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/main_window/status_bar_progress_widget.h"

namespace mtx::gui {

class StatusBarProgressWidgetPrivate {
  friend class StatusBarProgressWidget;

  std::unique_ptr<Ui::StatusBarProgressWidget> ui;
  int m_numPendingAuto{}, m_numPendingManual{}, m_numRunning{}, m_numOldWarnings{}, m_numCurrentWarnings{}, m_numOldErrors{}, m_numCurrentErrors{}, m_timerStep{};
  QTimer m_timer;
  QList<QPixmap> m_pixmaps;

  explicit StatusBarProgressWidgetPrivate()
    : ui{new Ui::StatusBarProgressWidget}
  {
  }
};

StatusBarProgressWidget::StatusBarProgressWidget(QWidget *parent)
  : QWidget{parent}
  , p_ptr{new StatusBarProgressWidgetPrivate{}}
{
  auto p = p_func();

  p->ui->setupUi(this);

  p->m_pixmaps << QIcon::fromTheme(Q("dialog-warning")).pixmap(16, 16);
  p->m_pixmaps << QIcon::fromTheme(Q("dialog-warning-grayscale")).pixmap(16, 16);
  p->m_pixmaps << QIcon::fromTheme(Q("dialog-error")).pixmap(16, 16);
  p->m_pixmaps << QIcon::fromTheme(Q("dialog-error-grayscale")).pixmap(16, 16);

  p->m_timer.setInterval(1000);

  connect(&p->m_timer, &QTimer::timeout, this, &StatusBarProgressWidget::updateWarningsAndErrorsIcons);
}

StatusBarProgressWidget::~StatusBarProgressWidget() {
}

void
StatusBarProgressWidget::reset() {
  setProgress(0, 0);
}

void
StatusBarProgressWidget::setProgress(int progress,
                                     int totalProgress) {
  auto p = p_func();

  p->ui->progress->setValue(progress);
  p->ui->totalProgress->setValue(totalProgress);
}

void
StatusBarProgressWidget::setJobStats(int numPendingAuto,
                                     int numPendingManual,
                                     int numRunning,
                                     int) {
  auto p                = p_func();
  p->m_numPendingAuto   = numPendingAuto;
  p->m_numPendingManual = numPendingManual;
  p->m_numRunning       = numRunning;

  setLabelTextsAndToolTip();
}

void
StatusBarProgressWidget::setNumUnacknowledgedWarningsOrErrors(int numOldWarnings,
                                                              int numCurrentWarnings,
                                                              int numOldErrors,
                                                              int numCurrentErrors) {
  auto p                  = p_func();
  p->m_numOldWarnings     = numOldWarnings;
  p->m_numCurrentWarnings = numCurrentWarnings;
  p->m_numOldErrors       = numOldErrors;
  p->m_numCurrentErrors   = numCurrentErrors;

  auto isActive           = p->m_timer.isActive();

  if (!isActive && (p->m_numOldWarnings || p->m_numCurrentWarnings || p->m_numOldErrors || p->m_numCurrentErrors)) {
    p->m_timerStep = 0;
    p->m_timer.start();

  } else if (isActive && !p->m_numOldWarnings && !p->m_numCurrentWarnings && !p->m_numOldErrors && !p->m_numCurrentErrors) {
    p->m_timer.stop();
    updateWarningsAndErrorsIcons();
  }

  setLabelTextsAndToolTip();
}

void
StatusBarProgressWidget::retranslateUi() {
  p_func()->ui->retranslateUi(this);

  setLabelTextsAndToolTip();
}

void
StatusBarProgressWidget::setLabelTextsAndToolTip() {
  auto p = p_func();

  p->ui->numJobsLabel->setText(QY("%1 automatic, %2 manual, %3 running").arg(p->m_numPendingAuto).arg(p->m_numPendingManual).arg(p->m_numRunning));
  p->ui->warningsLabel->setText(QNY("%1+%2 warning", "%1+%2 warnings", p->m_numCurrentWarnings + p->m_numOldWarnings).arg(p->m_numCurrentWarnings).arg(p->m_numOldWarnings));
  p->ui->errorsLabel->setText(QNY("%1+%2 error", "%1+%2 errors", p->m_numCurrentErrors + p->m_numOldErrors).arg(p->m_numCurrentErrors).arg(p->m_numOldErrors));

  auto format = Q("<p>%1</p>"
                  "<ul>"
                  "<li>%2</li>"
                  "<li>%3</li>"
                  "</ul>");

  auto toolTipWarnings = format
    .arg(QY("Number of warnings:"))
    .arg(QY("%1 since starting the program").arg(p->m_numCurrentWarnings))
    .arg(QY("%1 from earlier runs").arg(p->m_numOldWarnings));

  auto toolTipErrors = format
    .arg(QY("Number of errors:"))
    .arg(QY("%1 since starting the program").arg(p->m_numCurrentErrors))
    .arg(QY("%1 from earlier runs").arg(p->m_numOldErrors));

  setToolTip(toolTipWarnings + toolTipErrors);
}

void
StatusBarProgressWidget::updateWarningsAndErrorsIcons() {
  auto p             = p_func();
  auto warningOffset = !(p->m_numOldWarnings + p->m_numCurrentWarnings) || !(p->m_timerStep % 2) ? 1 : 0;
  auto errorOffset   = !(p->m_numOldErrors   + p->m_numCurrentErrors)   || !(p->m_timerStep % 2) ? 1 : 0;

  p->ui->warningsIconLabel->setPixmap(p->m_pixmaps[0 + warningOffset]);
  p->ui->errorsIconLabel  ->setPixmap(p->m_pixmaps[2 + errorOffset]);

  ++p->m_timerStep;
}

void
StatusBarProgressWidget::mouseReleaseEvent(QMouseEvent *event) {
  auto p = p_func();

  event->accept();

  QMenu menu{this};

  auto acknowledgeWarnings = new QAction{&menu};
  auto acknowledgeErrors   = new QAction{&menu};
  auto showJobQueue        = new QAction{&menu};
  auto showJobOutput       = new QAction{&menu};

  acknowledgeWarnings->setText(QY("Acknowledge all &warnings"));
  acknowledgeErrors->setText(QY("Acknowledge all &errors"));
  showJobQueue->setText(QY("Show job queue && access job logs"));
  showJobOutput->setText(QY("Show job output"));

  acknowledgeWarnings->setEnabled(!!(p->m_numOldWarnings + p->m_numCurrentWarnings));
  acknowledgeErrors->setEnabled(!!(p->m_numOldErrors + p->m_numCurrentErrors));

  connect(acknowledgeWarnings, &QAction::triggered, MainWindow::jobTool()->model(), &mtx::gui::Jobs::Model::acknowledgeAllWarnings);
  connect(acknowledgeErrors,   &QAction::triggered, MainWindow::jobTool()->model(), &mtx::gui::Jobs::Model::acknowledgeAllErrors);
  connect(showJobQueue,        &QAction::triggered, []{ MainWindow::get()->switchToTool(MainWindow::jobTool()); });
  connect(showJobOutput,       &QAction::triggered, []{
    auto tool = MainWindow::watchJobTool();
    tool->switchToCurrentJobTab();
    MainWindow::get()->switchToTool(tool);
  });

  menu.addAction(acknowledgeWarnings);
  menu.addAction(acknowledgeErrors);
  menu.addSeparator();
  menu.addAction(showJobQueue);
  menu.addAction(showJobOutput);

  menu.exec(mapToGlobal(event->pos()));
}

}
