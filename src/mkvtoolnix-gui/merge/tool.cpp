#include "common/common_pch.h"

// #include <QDragEnterEvent>
// #include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>
// #include <QMimeData>

#include "common/qt.h"
#include "mkvtoolnix-gui/forms/merge/tool.h"
#include "mkvtoolnix-gui/forms/main_window/main_window.h"
#include "mkvtoolnix-gui/merge/tab.h"
#include "mkvtoolnix-gui/merge/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/util/settings.h"

namespace mtx { namespace gui { namespace Merge {

using namespace mtx::gui;

Tool::Tool(QWidget *parent,
           QMenu *mergeMenu)
  : ToolBase{parent}
  , ui{new Ui::Tool}
  , m_mergeMenu{mergeMenu}
{
  // Setup UI controls.
  ui->setupUi(this);

  appendTab(new Tab{this});
  showMergeWidget();

  setupMenu();
  reconnectMenuActions();

  retranslateUi();
}

Tool::~Tool() {
}

void
Tool::setupMenu() {
  auto mwUi = MainWindow::get()->getUi();

  connect(mwUi->actionMergeNew,   &QAction::triggered, this, &Tool::newConfig);
  connect(mwUi->actionMergeOpen,  &QAction::triggered, this, &Tool::openConfig);
  connect(mwUi->actionMergeClose, &QAction::triggered, this, &Tool::closeCurrentTab);
}

void
Tool::reconnectMenuActions() {
  auto mwUi = MainWindow::get()->getUi();
  auto tab  = currentTab();

  mwUi->actionMergeSave->disconnect(SIGNAL(triggered()));
  mwUi->actionMergeSaveAs->disconnect(SIGNAL(triggered()));
  mwUi->actionMergeSaveOptionFile->disconnect(SIGNAL(triggered()));
  mwUi->actionMergeStartMuxing->disconnect(SIGNAL(triggered()));
  mwUi->actionMergeAddToJobQueue->disconnect(SIGNAL(triggered()));
  mwUi->actionMergeShowMkvmergeCommandLine->disconnect(SIGNAL(triggered()));

  if (!tab)
    return;

  connect(mwUi->actionMergeSave,                    &QAction::triggered, tab, &Tab::onSaveConfig);
  connect(mwUi->actionMergeSaveAs,                  &QAction::triggered, tab, &Tab::onSaveConfigAs);
  connect(mwUi->actionMergeSaveOptionFile,          &QAction::triggered, tab, &Tab::onSaveOptionFile);
  connect(mwUi->actionMergeStartMuxing,             &QAction::triggered, tab, &Tab::onStartMuxing);
  connect(mwUi->actionMergeAddToJobQueue,           &QAction::triggered, tab, &Tab::onAddToJobQueue);
  connect(mwUi->actionMergeShowMkvmergeCommandLine, &QAction::triggered, tab, &Tab::onShowCommandLine);
}

void
Tool::enableMenuActions() {
  auto mwUi   = MainWindow::get()->getUi();
  auto hasTab = !!currentTab();

  mwUi->actionMergeSave->setEnabled(hasTab);
  mwUi->actionMergeSaveAs->setEnabled(hasTab);
  mwUi->actionMergeSaveOptionFile->setEnabled(hasTab);
  mwUi->actionMergeClose->setEnabled(hasTab);
  mwUi->actionMergeStartMuxing->setEnabled(hasTab);
  mwUi->actionMergeAddToJobQueue->setEnabled(hasTab);
  mwUi->actionMergeShowMkvmergeCommandLine->setEnabled(hasTab);
}

void
Tool::showMergeWidget() {
  ui->stack->setCurrentWidget(ui->merges->count() ? ui->mergesPage : ui->noMergesPage);
  enableMenuActions();
}

void
Tool::toolShown() {
  MainWindow::get()->showTheseMenusOnly({ m_mergeMenu });
  showMergeWidget();
}

void
Tool::retranslateUi() {
  ui->retranslateUi(this);

  for (auto idx = 0, numTabs = ui->merges->count(); idx < numTabs; ++idx)
    static_cast<Tab *>(ui->merges->widget(idx))->retranslateUi();
}

Tab *
Tool::currentTab() {
  return static_cast<Tab *>(ui->merges->widget(ui->merges->currentIndex()));
}

Tab *
Tool::appendTab(Tab *tab) {
  connect(tab, &Tab::removeThisTab, this, &Tool::closeSendingTab);
  connect(tab, &Tab::titleChanged,  this, &Tool::tabTitleChanged);

  ui->merges->addTab(tab, tab->title());
  ui->merges->setCurrentIndex(ui->merges->count() - 1);

  showMergeWidget();
  reconnectMenuActions();

  return tab;
}

void
Tool::newConfig() {
  appendTab(new Tab{this});
}

void
Tool::openConfig() {
  auto &settings = Util::Settings::get();
  auto fileName  = QFileDialog::getOpenFileName(this, QY("Open settings file"), settings.m_lastConfigDir.path(), QY("MKVToolnix GUI config files") + Q(" (*.mtxcfg);;") + QY("All files") + Q(" (*)"));
  if (fileName.isEmpty())
    return;

  settings.m_lastConfigDir = QFileInfo{fileName}.path();
  settings.save();

  appendTab(new Tab{this})
   ->load(fileName);
}

void
Tool::closeTab(int index) {
  if ((0  > index) || (ui->merges->count() <= index))
    return;

  auto tab = static_cast<Tab *>(ui->merges->widget(index));

  // TODO: Tool::closeTab: hasBeenModified
  // if (tab->hasBeenModified()) {
  //   auto answer = QMessageBox::question(this, QY("File has been modified"), QY("The file »%1« has been modified. Do you really want to close? All changes will be lost.").arg(QFileInfo{tab->getFileName()}.fileName()),
  //                                       QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
  //   if (answer != QMessageBox::Yes)
  //     return;
  // }

  ui->merges->removeTab(index);
  delete tab;

  showMergeWidget();
}

void
Tool::closeCurrentTab() {
  closeTab(ui->merges->currentIndex());
}

void
Tool::closeSendingTab() {
  auto idx = ui->merges->indexOf(dynamic_cast<Tab *>(sender()));
  if (-1 != idx)
    closeTab(idx);
}

void
Tool::tabTitleChanged() {
  auto tab = dynamic_cast<Tab *>(sender());
  auto idx = ui->merges->indexOf(tab);
  if (tab && (-1 != idx))
    ui->merges->setTabText(idx, tab->title());
}

}}}
