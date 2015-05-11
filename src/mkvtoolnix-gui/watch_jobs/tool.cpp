#include "common/common_pch.h"

#include "common/qt.h"
#include "mkvtoolnix-gui/forms/main_window/main_window.h"
#include "mkvtoolnix-gui/forms/watch_jobs/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/watch_jobs/tool.h"
#include "mkvtoolnix-gui/watch_jobs/tab.h"

namespace mtx { namespace gui { namespace WatchJobs {

Tool::Tool(QWidget *parent)
  : ToolBase{parent}
  , ui{new Ui::Tool}
{
  // Setup UI controls.
  ui->setupUi(this);

  m_currentJobTab = new Tab{ui->widgets};
  ui->widgets->insertTab(0, m_currentJobTab, Q(""));
}

Tool::~Tool() {
}

void
Tool::retranslateUi() {
  ui->retranslateUi(this);
  ui->widgets->setTabText(0, QY("Current job"));

  m_currentJobTab->retranslateUi();
}

Tab *
Tool::currentJobTab() {
  return m_currentJobTab;
}

void
Tool::toolShown() {
}

}}}
