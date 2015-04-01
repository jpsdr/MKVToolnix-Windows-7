#include "common/common_pch.h"

#include "common/qt.h"
#include "mkvtoolnix-gui/forms/main_window.h"
#include "mkvtoolnix-gui/forms/watch_job_container_widget.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/watch_job_container_widget/watch_job_container_widget.h"
#include "mkvtoolnix-gui/watch_job_container_widget/watch_job_widget.h"

WatchJobContainerWidget::WatchJobContainerWidget(QWidget *parent)
  : ToolBase{parent}
  , ui{new Ui::WatchJobContainerWidget}
  , m_currentJobWidget{}
{
  // Setup UI controls.
  ui->setupUi(this);

  m_currentJobWidget = new WatchJobWidget{ui->widgets};
  ui->widgets->insertTab(0, m_currentJobWidget, Q(""));
}

WatchJobContainerWidget::~WatchJobContainerWidget() {
}

void
WatchJobContainerWidget::retranslateUi() {
  ui->widgets->setTabText(0, QY("Current job"));
}

WatchJobWidget *
WatchJobContainerWidget::currentJobWidget() {
  return m_currentJobWidget;
}

void
WatchJobContainerWidget::toolShown() {
  auto win = MainWindow::get();
  win->showTheseMenusOnly({ win->getUi()->menuMerge });
}
