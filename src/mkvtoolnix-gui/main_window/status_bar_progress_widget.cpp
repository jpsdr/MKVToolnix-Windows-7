#include "common/common_pch.h"

#include <QAction>
#include <QMenu>
#include <QMouseEvent>
#include <QTimer>

#include "common/qt.h"
#include "mkvtoolnix-gui/forms/main_window/status_bar_progress_widget.h"
#include "mkvtoolnix-gui/jobs/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/main_window/status_bar_progress_widget.h"

namespace mtx { namespace gui {

class StatusBarProgressWidgetPrivate {
  friend class StatusBarProgressWidget;

  std::unique_ptr<Ui::StatusBarProgressWidget> ui;
  int m_numPendingAuto{}, m_numPendingManual{}, m_numRunning{}, m_numWarnings{}, m_numErrors{}, m_timerStep{};
  QTimer m_timer;
  QList<QPixmap> m_pixmaps;

  explicit StatusBarProgressWidgetPrivate()
    : ui{new Ui::StatusBarProgressWidget}
  {
  }
};

StatusBarProgressWidget::StatusBarProgressWidget(QWidget *parent)
  : QWidget{parent}
  , d_ptr{new StatusBarProgressWidgetPrivate{}}
{
  Q_D(StatusBarProgressWidget);

  d->ui->setupUi(this);

  d->m_pixmaps << QPixmap{Q(":/icons/16x16/dialog-warning.png")};
  d->m_pixmaps << QPixmap{Q(":/icons/16x16/dialog-warning-grayscale.png")};
  d->m_pixmaps << QPixmap{Q(":/icons/16x16/dialog-error.png")};
  d->m_pixmaps << QPixmap{Q(":/icons/16x16/dialog-error-grayscale.png")};

  d->m_timer.setInterval(1000);

  connect(&d->m_timer, &QTimer::timeout, this, &StatusBarProgressWidget::updateWarningsAndErrorsIcons);
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
  Q_D(StatusBarProgressWidget);

  d->ui->progress->setValue(progress);
  d->ui->totalProgress->setValue(totalProgress);
}

void
StatusBarProgressWidget::setJobStats(int numPendingAuto,
                                     int numPendingManual,
                                     int numRunning,
                                     int) {
  Q_D(StatusBarProgressWidget);

  d->m_numPendingAuto   = numPendingAuto;
  d->m_numPendingManual = numPendingManual;
  d->m_numRunning       = numRunning;

  setLabelTexts();
}

void
StatusBarProgressWidget::setNumUnacknowledgedWarningsOrErrors(int numWarnings,
                                                              int numErrors) {
  Q_D(StatusBarProgressWidget);

  d->m_numWarnings = numWarnings;
  d->m_numErrors   = numErrors;

  auto isActive = d->m_timer.isActive();

  if (!isActive && (d->m_numWarnings || d->m_numErrors)) {
    d->m_timerStep = 0;
    d->m_timer.start();

  } else if (isActive && !d->m_numWarnings && !d->m_numErrors) {
    d->m_timer.stop();
    updateWarningsAndErrorsIcons();
  }

  setLabelTexts();
}

void
StatusBarProgressWidget::retranslateUi() {
  Q_D(StatusBarProgressWidget);

  d->ui->retranslateUi(this);

  setLabelTexts();
}

void
StatusBarProgressWidget::setLabelTexts() {
  Q_D(StatusBarProgressWidget);

  d->ui->numJobsLabel->setText(QY("%1 automatic, %2 manual, %3 running").arg(d->m_numPendingAuto).arg(d->m_numPendingManual).arg(d->m_numRunning));
  d->ui->warningsLabel->setText(QNY("%1 warning", "%1 warnings", d->m_numWarnings).arg(d->m_numWarnings));
  d->ui->errorsLabel  ->setText(QNY("%1 error",   "%1 errors",   d->m_numErrors)  .arg(d->m_numErrors));
}

void
StatusBarProgressWidget::updateWarningsAndErrorsIcons() {
  Q_D(StatusBarProgressWidget);

  auto warningOffset = !d->m_numWarnings || !(d->m_timerStep % 2) ? 1 : 0;
  auto errorOffset   = !d->m_numErrors   || !(d->m_timerStep % 2) ? 1 : 0;

  d->ui->warningsIconLabel->setPixmap(d->m_pixmaps[0 + warningOffset]);
  d->ui->errorsIconLabel  ->setPixmap(d->m_pixmaps[2 + errorOffset]);

  ++d->m_timerStep;
}

void
StatusBarProgressWidget::mouseReleaseEvent(QMouseEvent *event) {
  Q_D(StatusBarProgressWidget);

  event->accept();

  QMenu menu{this};

  auto acknowledgeWarnings = new QAction{&menu};
  auto acknowledgeErrors   = new QAction{&menu};

  acknowledgeWarnings->setText(QY("Acknowledge all &warnings"));
  acknowledgeErrors->setText(QY("Acknowledge all &errors"));

  acknowledgeWarnings->setEnabled(!!d->m_numWarnings);
  acknowledgeErrors->setEnabled(!!d->m_numErrors);

  connect(acknowledgeWarnings, &QAction::triggered, MainWindow::jobTool()->model(), &mtx::gui::Jobs::Model::acknowledgeAllWarnings);
  connect(acknowledgeErrors,   &QAction::triggered, MainWindow::jobTool()->model(), &mtx::gui::Jobs::Model::acknowledgeAllErrors);

  menu.addAction(acknowledgeWarnings);
  menu.addAction(acknowledgeErrors);

  menu.exec(mapToGlobal(event->pos()));
}

}}
