#include "common/common_pch.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>

#include "common/qt.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/forms/merge/tool.h"
#include "mkvtoolnix-gui/forms/main_window/main_window.h"
#include "mkvtoolnix-gui/jobs/tool.h"
#include "mkvtoolnix-gui/merge/tab.h"
#include "mkvtoolnix-gui/merge/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/util/file_dialog.h"
#include "mkvtoolnix-gui/util/message_box.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/string.h"
#include "mkvtoolnix-gui/util/widget.h"

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

  MainWindow::get()->registerSubWindowWidget(*this, *ui->merges);
}

Tool::~Tool() {
}

void
Tool::setupUi() {
  setupTabPositions();

  showMergeWidget();

  retranslateUi();
}

void
Tool::setupActions() {
  auto mw   = MainWindow::get();
  auto mwUi = MainWindow::getUi();

  connect(m_mergeMenu,                                &QMenu::aboutToShow,               this, &Tool::enableMenuActions);
  connect(m_mergeMenu,                                &QMenu::aboutToHide,               this, &Tool::enableCopyMenuActions);

  connect(mwUi->actionMergeNew,                       &QAction::triggered,               this, &Tool::newConfig);
  connect(mwUi->actionMergeOpen,                      &QAction::triggered,               this, &Tool::openConfig);
  connect(mwUi->actionMergeClose,                     &QAction::triggered,               this, &Tool::closeCurrentTab);
  connect(mwUi->actionMergeCloseAll,                  &QAction::triggered,               this, &Tool::closeAllTabs);
  connect(mwUi->actionMergeSave,                      &QAction::triggered,               this, &Tool::saveConfig);
  connect(mwUi->actionMergeSaveAll,                   &QAction::triggered,               this, &Tool::saveAllConfigs);
  connect(mwUi->actionMergeSaveAs,                    &QAction::triggered,               this, &Tool::saveConfigAs);
  connect(mwUi->actionMergeSaveOptionFile,            &QAction::triggered,               this, &Tool::saveOptionFile);
  connect(mwUi->actionMergeStartMuxing,               &QAction::triggered,               this, &Tool::startMuxing);
  connect(mwUi->actionMergeStartMuxingAll,            &QAction::triggered,               this, &Tool::startMuxingAll);
  connect(mwUi->actionMergeAddToJobQueue,             &QAction::triggered,               this, &Tool::addToJobQueue);
  connect(mwUi->actionMergeAddAllToJobQueue,          &QAction::triggered,               this, &Tool::addAllToJobQueue);
  connect(mwUi->actionMergeShowMkvmergeCommandLine,   &QAction::triggered,               this, &Tool::showCommandLine);
  connect(mwUi->actionMergeCopyFirstFileNameToTitle,  &QAction::triggered,               this, &Tool::copyFirstFileNameToTitle);
  connect(mwUi->actionMergeCopyOutputFileNameToTitle, &QAction::triggered,               this, &Tool::copyOutputFileNameToTitle);
  connect(mwUi->actionMergeCopyTitleToOutputFileName, &QAction::triggered,               this, &Tool::copyTitleToOutputFileName);

  connect(ui->merges,                                 &QTabWidget::tabCloseRequested,    this, &Tool::closeTab);
  connect(ui->newFileButton,                          &QPushButton::clicked,             this, &Tool::newConfig);
  connect(ui->openFileButton,                         &QPushButton::clicked,             this, &Tool::openConfig);

  connect(mw,                                         &MainWindow::preferencesChanged,   this, &Tool::setupTabPositions);
  connect(mw,                                         &MainWindow::preferencesChanged,   this, &Tool::retranslateUi);

  connect(App::instance(),                            &App::addingFilesToMergeRequested, this, &Tool::addMultipleFilesFromCommandLine);
  connect(App::instance(),                            &App::openConfigFilesRequested,    this, &Tool::openMultipleConfigFilesFromCommandLine);
}

void
Tool::enableMenuActions() {
  auto mwUi   = MainWindow::getUi();
  auto tab    = currentTab();
  auto hasTab = tab && tab->isEnabled();

  mwUi->actionMergeSave->setEnabled(hasTab);
  mwUi->actionMergeSaveAs->setEnabled(hasTab);
  mwUi->actionMergeSaveOptionFile->setEnabled(hasTab);
  mwUi->actionMergeClose->setEnabled(hasTab);
  mwUi->actionMergeStartMuxing->setEnabled(hasTab);
  mwUi->actionMergeAddToJobQueue->setEnabled(hasTab);
  mwUi->actionMergeShowMkvmergeCommandLine->setEnabled(hasTab);
  mwUi->actionMergeCopyFirstFileNameToTitle->setEnabled(hasTab && tab->hasSourceFiles());
  mwUi->actionMergeCopyOutputFileNameToTitle->setEnabled(hasTab && tab->hasDestinationFileName());
  mwUi->actionMergeCopyTitleToOutputFileName->setEnabled(hasTab && tab->hasTitle());
  mwUi->menuMergeAll->setEnabled(hasTab);
  mwUi->actionMergeSaveAll->setEnabled(hasTab);
  mwUi->actionMergeCloseAll->setEnabled(hasTab);
  mwUi->actionMergeStartMuxingAll->setEnabled(hasTab);
  mwUi->actionMergeAddAllToJobQueue->setEnabled(hasTab);
}

void
Tool::enableCopyMenuActions() {
  auto mwUi = MainWindow::getUi();

  mwUi->actionMergeCopyFirstFileNameToTitle->setEnabled(true);
  mwUi->actionMergeCopyOutputFileNameToTitle->setEnabled(true);
  mwUi->actionMergeCopyTitleToOutputFileName->setEnabled(true);
}

void
Tool::showMergeWidget() {
  ui->stack->setCurrentWidget(ui->merges->count() ? ui->mergesPage : ui->noMergesPage);
  enableMenuActions();
  enableCopyMenuActions();
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

  ui->merges->addTab(tab, Util::escape(tab->title(), Util::EscapeKeyboardShortcuts));
  ui->merges->setCurrentIndex(ui->merges->count() - 1);

  showMergeWidget();

  return tab;
}

void
Tool::newConfig() {
  appendTab(new Tab{this});
}

void
Tool::openConfig() {
  auto &settings = Util::Settings::get();
  auto fileName  = Util::getOpenFileName(this, QY("Open settings file"), settings.lastConfigDirPath(), QY("MKVToolNix GUI config files") + Q(" (*.mtxcfg);;") + QY("All files") + Q(" (*)"));
  if (fileName.isEmpty())
    return;

  openConfigFile(fileName);
}

void
Tool::openConfigFile(QString const &fileName) {
  Util::Settings::change([&fileName](Util::Settings &cfg) {
    cfg.m_lastConfigDir = QFileInfo{fileName}.path();
  });

  if (MainWindow::jobTool()->addJobFile(fileName)) {
    MainWindow::get()->setStatusBarMessage(QY("The job has been added to the job queue."));
    return;
  }

  auto tab = currentTab();
  if (tab && tab->isEmpty())
    tab->deleteLater();

  appendTab(new Tab{this})
    ->load(fileName);
}

void
Tool::openFromConfig(MuxConfig const &config) {
  appendTab(new Tab{this})
    ->cloneConfig(config);
}

bool
Tool::closeTab(int index) {
  if ((0  > index) || (ui->merges->count() <= index))
    return false;

  auto tab = static_cast<Tab *>(ui->merges->widget(index));

  if (Util::Settings::get().m_warnBeforeClosingModifiedTabs && tab->hasBeenModified()) {
    MainWindow::get()->switchToTool(this);
    ui->merges->setCurrentIndex(index);

    auto answer = Util::MessageBox::question(this)
      ->title(QY("Close modified file"))
      .text(QY("The file \"%1\" has been modified. Do you really want to close? All changes will be lost.").arg(tab->title()))
      .buttonLabel(QMessageBox::Yes, QY("&Close file"))
      .buttonLabel(QMessageBox::No,  QY("Cancel"))
      .exec();
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
  for (auto index = ui->merges->count(); index > 0; --index) {
    ui->merges->setCurrentIndex(index);
    if (!closeTab(index - 1))
      return false;
  }

  return true;
}

void
Tool::saveConfig() {
  auto tab = currentTab();
  if (tab)
    tab->onSaveConfig();
}

void
Tool::saveAllConfigs() {
  forEachTab([](Tab &tab) { tab.onSaveConfig(); });
}

void
Tool::saveConfigAs() {
  auto tab = currentTab();
  if (tab)
    tab->onSaveConfigAs();
}

void
Tool::saveOptionFile() {
  auto tab = currentTab();
  if (tab)
    tab->onSaveOptionFile();
}

void
Tool::startMuxing() {
  auto tab = currentTab();
  if (tab)
    tab->addToJobQueue(true);
}

void
Tool::startMuxingAll() {
  forEachTab([](Tab &tab) { tab.addToJobQueue(true); });
}

void
Tool::addToJobQueue() {
  auto tab = currentTab();
  if (tab)
    tab->addToJobQueue(false);
}

void
Tool::addAllToJobQueue() {
  forEachTab([](Tab &tab) { tab.addToJobQueue(false); });
}

void
Tool::showCommandLine() {
  auto tab = currentTab();
  if (tab)
    tab->onShowCommandLine();
}

void
Tool::copyFirstFileNameToTitle() {
  auto tab = currentTab();
  if (tab && tab->isEnabled())
    tab->onCopyFirstFileNameToTitle();
}

void
Tool::copyOutputFileNameToTitle() {
  auto tab = currentTab();
  if (tab && tab->isEnabled())
    tab->onCopyOutputFileNameToTitle();
}

void
Tool::copyTitleToOutputFileName() {
  auto tab = currentTab();
  if (tab && tab->isEnabled())
    tab->onCopyTitleToOutputFileName();
}

void
Tool::tabTitleChanged() {
  auto tab = dynamic_cast<Tab *>(sender());
  auto idx = ui->merges->indexOf(tab);
  if (tab && (-1 != idx))
    ui->merges->setTabText(idx, Util::escape(tab->title(), Util::EscapeKeyboardShortcuts));
}

void
Tool::dragEnterEvent(QDragEnterEvent *event) {
  m_filesDDHandler.handle(event, false);
}

void
Tool::dropEvent(QDropEvent *event) {
  if (m_filesDDHandler.handle(event, true))
    filesDropped(m_filesDDHandler.fileNames(), event->mouseButtons());
}

void
Tool::filesDropped(QStringList const &fileNames,
                   Qt::MouseButtons mouseButtons) {
  auto configExt  = Q(".mtxcfg");
  auto mediaFiles = QStringList{};

  for (auto const &fileName : fileNames)
    if (fileName.endsWith(configExt))
      openConfigFile(fileName);
    else
      mediaFiles << fileName;

  if (!mediaFiles.isEmpty())
    addMultipleFiles(mediaFiles, mouseButtons);
}

void
Tool::addMultipleFiles(QStringList const &fileNames,
                       Qt::MouseButtons mouseButtons) {
  if (!ui->merges->count())
    newConfig();

  auto tab = currentTab();
  Q_ASSERT(!!tab);

  tab->addFilesToBeAddedOrAppendedDelayed(fileNames, mouseButtons);
}

void
Tool::addMultipleFilesFromCommandLine(QStringList const &fileNames) {
  MainWindow::get()->switchToTool(this);
  addMultipleFiles(fileNames, Qt::NoButton);
}

void
Tool::openMultipleConfigFilesFromCommandLine(QStringList const &fileNames) {
  MainWindow::get()->switchToTool(this);
  for (auto const &fileName : fileNames)
    openConfigFile(fileName);
}

void
Tool::addMultipleFilesToNewSettings(QStringList const &fileNames,
                                    bool newSettingsForEachFile) {
  auto toProcess = fileNames;

  while (!toProcess.isEmpty()) {
    auto fileNamesToAdd = QStringList{};

    if (newSettingsForEachFile)
      fileNamesToAdd << toProcess.takeFirst();

    else {
      fileNamesToAdd = toProcess;
      toProcess.clear();
    }

    newConfig();

    auto tab = currentTab();
    Q_ASSERT(!!tab);

    tab->addFiles(fileNamesToAdd);
  }
}

void
Tool::setupTabPositions() {
  ui->merges->setTabPosition(Util::Settings::get().m_tabPosition);
}

void
Tool::addMergeTabIfNoneOpen() {
  if (!ui->merges->count())
    appendTab(new Tab{this});
}

void
Tool::forEachTab(std::function<void(Tab &)> const &worker) {
  auto currentIndex = ui->merges->currentIndex();

  for (auto index = 0, numTabs = ui->merges->count(); index < numTabs; ++index) {
    ui->merges->setCurrentIndex(index);
    worker(dynamic_cast<Tab &>(*ui->merges->widget(index)));
  }

  ui->merges->setCurrentIndex(currentIndex);
}

std::pair<QString, QString>
Tool::nextPreviousWindowActionTexts()
  const {
  return {
    QY("&Next multiplex settings"),
    QY("&Previous multiplex settings"),
  };
}

}}}
