#include "common/common_pch.h"

#include "common/qt.h"
#include "mkvtoolnix-gui/forms/main_window/status_bar_progress_widget.h"
#include "mkvtoolnix-gui/main_window/status_bar_progress_widget.h"

namespace mtx { namespace gui {

StatusBarProgressWidget::StatusBarProgressWidget(QWidget *parent)
  : QWidget{parent}
  , ui{new Ui::StatusBarProgressWidget}
{
  ui->setupUi(this);
}

StatusBarProgressWidget::~StatusBarProgressWidget() {
}

void
StatusBarProgressWidget::setProgress(int progress,
                                     int totalProgress) {
  ui->progress->setValue(progress);
  ui->totalProgress->setValue(totalProgress);
}

void
StatusBarProgressWidget::setJobStats(int numPendingAuto,
                                     int numPendingManual,
                                     int) {
  m_numPendingAuto   = numPendingAuto;
  m_numPendingManual = numPendingManual;

  ui->numJobsLabel->setText(QY("%1 automatic, %2 manual").arg(m_numPendingAuto).arg(m_numPendingManual));
}

void
StatusBarProgressWidget::retranslateUi() {
  ui->retranslateUi(this);

  ui->numJobsLabel->setText(QY("%1 automatic, %2 manual").arg(m_numPendingAuto).arg(m_numPendingManual));
}

}}
