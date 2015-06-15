#include "common/common_pch.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>

#include "common/qt.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/forms/merge/tool.h"
#include "mkvtoolnix-gui/forms/main_window/main_window.h"
#include "mkvtoolnix-gui/merge/tab.h"
#include "mkvtoolnix-gui/merge/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/util/message_box.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/util.h"

namespace mtx { namespace gui { namespace Merge {

using namespace mtx::gui;

Tool::Tool(QWidget *parent,
           QMenu *mergeMenu)
  : ToolBase{parent}
  , ui{new Ui::Tool}
  , m_mergeMenu{mergeMenu}
  , m_filesDDHandler{Util::FilesDragDropHandler::Mode::Remember}
{
  // Setup UI controls.
  ui->setupUi(this);

  appendTab(new Tab{this});
  showMergeWidget();

  setupActions();
  reconnectMenuActions();

  retranslateUi();
}

Tool::~Tool() {
}

void
Tool::setupActions() {
  auto mwUi = MainWindow::getUi();

  connect(mwUi->actionMergeNew,   &QAction::triggered,   this, &Tool::newConfig);
  connect(mwUi->actionMergeOpen,  &QAction::triggered,   this, &Tool::openConfig);
  connect(mwUi->actionMergeClose, &QAction::triggered,   this, &Tool::closeCurrentTab);

  connect(ui->newFileButton,      &QPushButton::clicked, this, &Tool::newConfig);
  connect(ui->openFileButton,     &QPushButton::clicked, this, &Tool::openConfig);
}

void
Tool::reconnectMenuActions() {
  auto mwUi = MainWindow::getUi();
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
  auto mwUi   = MainWindow::getUi();
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

  for (auto idx = 0, numTabs = ui->merges->count(); idx < numTabs; ++idx) {
    static_cast<Tab *>(ui->merges->widget(idx))->retranslateUi();
    auto button = Util::tabWidgetCloseTabButton(*ui->merges, idx);
    if (button)
      button->setToolTip(App::translate("CloseButton", "Close Tab"));
  }
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

  openConfigFile(fileName);
}

void
Tool::openConfigFile(QString const &fileName) {
  Util::Settings::change([&fileName](Util::Settings &cfg) {
    cfg.m_lastConfigDir = QFileInfo{fileName}.path();
  });

  appendTab(new Tab{this})
   ->load(fileName);
}

bool
Tool::closeTab(int index) {
  if ((0  > index) || (ui->merges->count() <= index))
    return false;

  auto tab = static_cast<Tab *>(ui->merges->widget(index));

  if (Util::Settings::get().m_warnBeforeClosingModifiedTabs && tab->hasBeenModified()) {
    MainWindow::get()->switchToTool(this);
    ui->merges->setCurrentIndex(index);

    auto answer = Util::MessageBox::question(this, QY("File has been modified"), QY("The file »%1« has been modified. Do you really want to close? All changes will be lost.").arg(tab->title()),
                                             QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes)
      return false;
  }

  ui->merges->removeTab(index);
  delete tab;

  showMergeWidget();

  return true;
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

bool
Tool::closeAllTabs() {
  for (auto index = ui->merges->count(); index > 0; --index)
    if (!closeTab(index - 1))
      return false;

  return true;
}

void
Tool::tabTitleChanged() {
  auto tab = dynamic_cast<Tab *>(sender());
  auto idx = ui->merges->indexOf(tab);
  if (tab && (-1 != idx))
    ui->merges->setTabText(idx, tab->title());
}

void
Tool::dragEnterEvent(QDragEnterEvent *event) {
  m_filesDDHandler.handle(event, false);
}

void
Tool::dropEvent(QDropEvent *event) {
  if (m_filesDDHandler.handle(event, true))
    filesDropped(m_filesDDHandler.fileNames());
}

void
Tool::filesDropped(QStringList const &fileNames) {
  auto configExt  = Q(".mtxcfg");
  auto mediaFiles = QStringList{};

  for (auto const &fileName : fileNames)
    if (fileName.endsWith(configExt))
      openConfigFile(fileName);
    else
      mediaFiles << fileName;

  if (mediaFiles.isEmpty())
    return;

  if (!ui->merges->count())
    newConfig();

  auto tab = currentTab();
  Q_ASSERT(!!tab);

  tab->addFilesToBeAddedOrAppendedDelayed(mediaFiles);
}

}}}
