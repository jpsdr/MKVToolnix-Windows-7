#include "common/common_pch.h"

#include <QMenu>
#include <QPushButton>

#include "common/qt.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/forms/main_window/main_window.h"
#include "mkvtoolnix-gui/forms/watch_jobs/tool.h"
#include "mkvtoolnix-gui/jobs/job.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/widget.h"
#include "mkvtoolnix-gui/watch_jobs/tool.h"
#include "mkvtoolnix-gui/watch_jobs/tab.h"

namespace mtx { namespace gui { namespace WatchJobs {

Tool::Tool(QWidget *parent,
           QMenu *jobOutputMenu)
  : ToolBase{parent}
  , ui{new Ui::Tool}
  , m_jobOutputMenu{jobOutputMenu}
{
  // Setup UI controls.
  ui->setupUi(this);

  setupTabPositions();

  auto mw   = MainWindow::get();
  auto mwUi = MainWindow::getUi();

  connect(mwUi->actionJobOutputSave,  &QAction::triggered,             this, &Tool::saveCurrentTabOutput);
  connect(mwUi->actionJobOutputClose, &QAction::triggered,             this, &Tool::closeCurrentTab);
  connect(ui->widgets,                &QTabWidget::tabCloseRequested,  this, &Tool::closeTab);
  connect(ui->widgets,                &QTabWidget::currentChanged,     this, &Tool::enableMenuActions);
  connect(mw,                         &MainWindow::preferencesChanged, this, &Tool::setupTabPositions);

  m_currentJobTab = new Tab{ui->widgets};
  ui->widgets->insertTab(0, m_currentJobTab, Q(""));

  auto button = Util::tabWidgetCloseTabButton(*ui->widgets, 0);
  if (button)
    button->resize(0, 0);

  enableMenuActions();
}

Tool::~Tool() {
}

void
Tool::retranslateUi() {
  ui->retranslateUi(this);
  ui->widgets->setTabText(0, QY("Current job"));

  m_currentJobTab->retranslateUi();

  for (auto idx = 0, numTabs = ui->widgets->count(); idx < numTabs; ++idx) {
    auto button = Util::tabWidgetCloseTabButton(*ui->widgets, idx);
    if (button)
      button->setToolTip(App::translate("CloseButton", "Close Tab"));
  }
}

Tab *
Tool::currentJobTab() {
  return m_currentJobTab;
}

Tab *
Tool::currentTab() {
  return static_cast<Tab *>(ui->widgets->currentWidget());
}

void
Tool::toolShown() {
  MainWindow::get()->showTheseMenusOnly({ m_jobOutputMenu });
  enableMenuActions();
}

void
Tool::viewOutput(mtx::gui::Jobs::Job &job) {
  auto idx = tabIndexForJobID(job.id());
  if (0 < idx) {
    ui->widgets->setCurrentIndex(idx);
    return;
  }

  auto tab = new Tab{ui->widgets};
  tab->connectToJob(job);
  tab->setInitialDisplay(job);

  ui->widgets->addTab(tab, job.description());
  ui->widgets->setCurrentIndex(ui->widgets->count() - 1);
}

int
Tool::tabIndexForJobID(uint64_t id)
  const {
  for (int idx = 1, numTabs = ui->widgets->count(); idx < numTabs; ++idx) {
    auto tab = static_cast<Tab *>(ui->widgets->widget(idx));
    if (tab->id() == id)
      return idx;
  }

  return -1;
}

void
Tool::closeTab(int idx) {
  if (idx)
    ui->widgets->removeTab(idx);
}

void
Tool::closeCurrentTab() {
  closeTab(ui->widgets->currentIndex());
}

void
Tool::saveCurrentTabOutput() {
  auto tab = currentTab();
  if (tab && tab->isSaveOutputEnabled())
    tab->onSaveOutput();
}

void
Tool::enableMenuActions() {
  auto mwUi = MainWindow::getUi();

  mwUi->actionJobOutputSave->setEnabled(currentTab()->isSaveOutputEnabled());
  mwUi->actionJobOutputClose->setEnabled(ui->widgets->currentIndex() > 0);
}

void
Tool::setupTabPositions() {
  ui->widgets->setTabPosition(Util::Settings::get().m_tabPosition);
}

}}}
