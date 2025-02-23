#include "common/common_pch.h"

#include <QClipboard>
#include <QDir>
#include <QDirIterator>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>
#include <QProgressDialog>

#include "common/bluray/util.h"
#include "common/fs_sys_helpers.h"
#include "common/path.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/forms/merge/tool.h"
#include "mkvtoolnix-gui/forms/main_window/main_window.h"
#include "mkvtoolnix-gui/jobs/tool.h"
#include "mkvtoolnix-gui/merge/adding_appending_files_dialog.h"
#include "mkvtoolnix-gui/merge/adding_directories_dialog.h"
#include "mkvtoolnix-gui/merge/ask_scan_for_playlists_dialog.h"
#include "mkvtoolnix-gui/merge/command_line_dialog.h"
#include "mkvtoolnix-gui/merge/file_identification_thread.h"
#include "mkvtoolnix-gui/merge/file_identification_pack.h"
#include "mkvtoolnix-gui/merge/select_disc_library_information_dialog.h"
#include "mkvtoolnix-gui/merge/select_playlist_dialog.h"
#include "mkvtoolnix-gui/merge/source_file.h"
#include "mkvtoolnix-gui/merge/tab.h"
#include "mkvtoolnix-gui/merge/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/util/config_file.h"
#include "mkvtoolnix-gui/util/file.h"
#include "mkvtoolnix-gui/util/file_dialog.h"
#include "mkvtoolnix-gui/util/files_drag_drop_handler.h"
#include "mkvtoolnix-gui/util/message_box.h"
#include "mkvtoolnix-gui/util/modify_tracks_submenu.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/string.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace boost::filesystem {

uint
qHash(boost::filesystem::path const &path) {
  return qHash(Q(path.string()));
}

}

namespace mtx::gui::Merge {

namespace {

struct ResolvedDirectories {
  using Dir = std::pair<QString, QStringList>;
  QVector<Dir> m_directories;
  QStringList m_files;
};

ResolvedDirectories
resolveDirectoriesToContainedFilesAndDirectories(QStringList const &namesToCheck) {
  ResolvedDirectories resolvedDirs;

  for (auto const &nameToCheck : namesToCheck) {
    auto info = QFileInfo{nameToCheck};
    if (!info.exists())
      continue;

    if (info.isFile()) {
      resolvedDirs.m_files << nameToCheck;
      continue;
    }

    if (!info.isDir())
      continue;

    QStringList newFileNames;
    QDirIterator it{nameToCheck, QDirIterator::Subdirectories};

    while (it.hasNext()) {
      it.next();
      info = it.fileInfo();

      if (info.isFile())
        newFileNames << info.absoluteFilePath();
    }

    newFileNames.sort(Qt::CaseInsensitive);

    if (!newFileNames.isEmpty())
      resolvedDirs.m_directories << std::make_pair(nameToCheck, newFileNames);
  }

  return resolvedDirs;
}

}

using namespace mtx::gui;

class ToolPrivate {
  friend class Tool;

  // UI stuff:
  std::unique_ptr<Ui::Tool> ui;
  QMenu *mergeMenu{}, *languageShortcutsMenu{};
  mtx::gui::Util::FilesDragDropHandler filesDDHandler{Util::FilesDragDropHandler::Mode::Remember};

  FileIdentificationThread *identifier{};
  QProgressDialog *scanningDirectoryDialog{}, *identifyingFilesDialog{};

  QHash<Tab *, int> lastAddAppendFileNum;

  QString m_singleConfigFileName;

  mtx::gui::Util::ModifyTracksSubmenu modifyTracksSubmenu;

  ToolPrivate(QWidget *p_parent, QMenu *p_mergeMenu);
};

ToolPrivate::ToolPrivate(QWidget *p_parent,
                         QMenu *p_mergeMenu)
  : ui{new Ui::Tool}
  , mergeMenu{p_mergeMenu}
  , identifier{new FileIdentificationThread{p_parent}}
{
}

// ------------------------------------------------------------

Tool::Tool(QWidget *parent,
           QMenu *mergeMenu)
  : ToolBase{parent}
  , p_ptr{new ToolPrivate{this, mergeMenu}}
{
  auto &p = *p_func();

  // Setup UI controls.
  p.ui->setupUi(this);

  setupFileIdentificationThread();

  MainWindow::get()->registerSubWindowWidget(*this, *p.ui->merges);
}

Tool::~Tool() {
  auto &p = *p_func();

  p.identifier->abortPlaylistScan();
  p.identifier->quit();
  p.identifier->wait();
}

void
Tool::setupUi() {
  auto &p = *p_func();

  setupModifySelectedTracksMenu();

  Util::setupTabWidgetHeaders(*p.ui->merges);

  showMergeWidget();

  retranslateUi();
}

void
Tool::setupModifySelectedTracksMenu() {
  auto &p                 = *p_func();
  auto mwUi               = MainWindow::getUi();
  p.languageShortcutsMenu = new QMenu{this};

  mwUi->menuMergeModifySelectedTracks->addMenu(p.languageShortcutsMenu);
  mwUi->menuMergeModifySelectedTracks->addSeparator();

  p.modifyTracksSubmenu.setupTrack(*mwUi->menuMergeModifySelectedTracks);
  p.modifyTracksSubmenu.setupLanguage(*p.languageShortcutsMenu);
}

void
Tool::setupActions() {
  auto &p   = *p_func();
  auto mw   = MainWindow::get();
  auto mwUi = MainWindow::getUi();
  auto &mts = p.modifyTracksSubmenu;

  connect(p.mergeMenu,                                &QMenu::aboutToShow,                                 this, &Tool::enableMenuActions);
  connect(p.mergeMenu,                                &QMenu::aboutToHide,                                 this, &Tool::enableCopyMenuActions);

  connect(mwUi->actionMergeNew,                       &QAction::triggered,                                 this, &Tool::appendNewTab);
  connect(mwUi->actionMergeOpen,                      &QAction::triggered,                                 this, &Tool::openConfig);
  connect(mwUi->actionMergeClose,                     &QAction::triggered,                                 this, &Tool::closeCurrentTab);
  connect(mwUi->actionMergeCloseAll,                  &QAction::triggered,                                 this, &Tool::closeAllTabs);
  connect(mwUi->actionMergeSave,                      &QAction::triggered,                                 this, &Tool::saveConfig);
  connect(mwUi->actionMergeSaveAll,                   &QAction::triggered,                                 this, &Tool::saveAllConfigs);
  connect(mwUi->actionMergeSaveAs,                    &QAction::triggered,                                 this, &Tool::saveConfigAs);
  connect(mwUi->actionMergeSaveAllSingle,             &QAction::triggered,                                 this, &Tool::saveAllConfigsToSingle);
  connect(mwUi->actionMergeSaveAllSingleAs,           &QAction::triggered,                                 this, &Tool::saveAllConfigsToSingleAs);
  connect(mwUi->actionMergeSaveOptionFile,            &QAction::triggered,                                 this, &Tool::saveOptionFile);
  connect(mwUi->actionMergeStartMuxing,               &QAction::triggered,                                 this, &Tool::startMuxing);
  connect(mwUi->actionMergeStartMuxingAll,            &QAction::triggered,                                 this, &Tool::startMuxingAll);
  connect(mwUi->actionMergeAddToJobQueue,             &QAction::triggered,                                 this, &Tool::addToJobQueue);
  connect(mwUi->actionMergeAddAllToJobQueue,          &QAction::triggered,                                 this, &Tool::addAllToJobQueue);
  connect(mwUi->actionMergeShowMkvmergeCommandLine,   &QAction::triggered,                                 this, &Tool::showCommandLine);
  connect(mwUi->actionMergeCopyFirstFileNameToTitle,  &QAction::triggered,                                 this, &Tool::copyFirstFileNameToTitle);
  connect(mwUi->actionMergeCopyOutputFileNameToTitle, &QAction::triggered,                                 this, &Tool::copyOutputFileNameToTitle);
  connect(mwUi->actionMergeCopyTitleToOutputFileName, &QAction::triggered,                                 this, &Tool::copyTitleToOutputFileName);
  connect(mwUi->actionMergeAddFilesFromClipboard,     &QAction::triggered,                                 this, &Tool::addFilesFromClipboard);

  connect(mts.m_toggleDefaultTrackFlag,               &QAction::triggered,                                 this, &Tool::toggleTrackFlag);
  connect(mts.m_toggleForcedDisplayFlag,              &QAction::triggered,                                 this, &Tool::toggleTrackFlag);
  connect(mts.m_toggleTrackEnabledFlag,               &QAction::triggered,                                 this, &Tool::toggleTrackFlag);
  connect(mts.m_toggleCommentaryFlag,                 &QAction::triggered,                                 this, &Tool::toggleTrackFlag);
  connect(mts.m_toggleOriginalFlag,                   &QAction::triggered,                                 this, &Tool::toggleTrackFlag);
  connect(mts.m_toggleHearingImpairedFlag,            &QAction::triggered,                                 this, &Tool::toggleTrackFlag);
  connect(mts.m_toggleVisualImpairedFlag,             &QAction::triggered,                                 this, &Tool::toggleTrackFlag);
  connect(mts.m_toggleTextDescriptionsFlag,           &QAction::triggered,                                 this, &Tool::toggleTrackFlag);
  connect(&mts,                                       &Util::ModifyTracksSubmenu::languageChangeRequested, this, &Tool::changeTrackLanguage);

  connect(p.ui->merges,                               &QTabWidget::tabCloseRequested,                      this, &Tool::closeTab);
  connect(p.ui->newFileButton,                        &QPushButton::clicked,                               this, &Tool::appendNewTab);
  connect(p.ui->openFileButton,                       &QPushButton::clicked,                               this, &Tool::openConfig);

  connect(mw,                                         &MainWindow::preferencesChanged,                     this, &Tool::applyPreferences);

  connect(App::instance(),                            &App::addingFilesToMergeRequested,                   this, &Tool::handleFilesFromCommandLine);
  connect(App::instance(),                            &App::openConfigFilesRequested,                      this, &Tool::openMultipleConfigFilesFromCommandLine);
}

void
Tool::setupFileIdentificationThread() {
  auto &p      = *p_func();
  auto &worker = p.identifier->worker();

  connect(&worker, &FileIdentificationWorker::queueStarted,               this, &Tool::fileIdentificationStarted);
  connect(&worker, &FileIdentificationWorker::queueFinished,              this, &Tool::fileIdentificationFinished);
  connect(&worker, &FileIdentificationWorker::queueProgressChanged,       this, &Tool::updateFileIdentificationProgress);
  connect(&worker, &FileIdentificationWorker::packIdentified,             this, &Tool::handleIdentifiedFiles);
  connect(&worker, &FileIdentificationWorker::identificationFailed,       this, &Tool::showFileIdentificationError);
  connect(&worker, &FileIdentificationWorker::playlistScanStarted,        this, &Tool::showScanningPlaylistDialog);
  connect(&worker, &FileIdentificationWorker::playlistScanFinished,       this, &Tool::hideScanningDirectoryDialog);
  connect(&worker, &FileIdentificationWorker::playlistScanDecisionNeeded, this, &Tool::selectScanPlaylistPolicy);
  connect(&worker, &FileIdentificationWorker::playlistSelectionNeeded,    this, &Tool::selectPlaylistToAdd);

  p.identifier->start();
}

void
Tool::applyPreferences() {
  auto &p = *p_func();

  Util::setupTabWidgetHeaders(*p.ui->merges);
  p.modifyTracksSubmenu.setupLanguage(*p.languageShortcutsMenu);
  retranslateUi();
}

void
Tool::enableMenuActions() {
  auto &p          = *p_func();
  auto mwUi        = MainWindow::getUi();
  auto tab         = currentTab();
  auto hasTab      = !!tab;
  auto identifying = !p.ui->overlordWidget->isEnabled();

  mwUi->actionMergeNew->setEnabled(!identifying);
  mwUi->actionMergeOpen->setEnabled(!identifying);
  mwUi->actionMergeSave->setEnabled(!identifying && hasTab);
  mwUi->actionMergeSaveAs->setEnabled(!identifying && hasTab);
  mwUi->actionMergeSaveAllSingle->setEnabled(!identifying && hasTab);
  mwUi->actionMergeSaveAllSingleAs->setEnabled(!identifying && hasTab);
  mwUi->actionMergeSaveOptionFile->setEnabled(!identifying && hasTab);
  mwUi->actionMergeClose->setEnabled(!identifying && hasTab);
  mwUi->actionMergeStartMuxing->setEnabled(!identifying && hasTab);
  mwUi->actionMergeAddToJobQueue->setEnabled(!identifying && hasTab);
  mwUi->actionMergeShowMkvmergeCommandLine->setEnabled(!identifying && hasTab);
  mwUi->actionMergeCopyFirstFileNameToTitle->setEnabled(!identifying && hasTab && tab->hasSourceFiles());
  mwUi->actionMergeCopyOutputFileNameToTitle->setEnabled(!identifying && hasTab && tab->hasDestinationFileName());
  mwUi->actionMergeCopyTitleToOutputFileName->setEnabled(!identifying && hasTab && tab->hasTitle());
  mwUi->menuMergeAll->setEnabled(!identifying && hasTab);
  mwUi->actionMergeSaveAll->setEnabled(!identifying && hasTab);
  mwUi->actionMergeCloseAll->setEnabled(!identifying && hasTab);
  mwUi->actionMergeStartMuxingAll->setEnabled(!identifying && hasTab);
  mwUi->actionMergeAddAllToJobQueue->setEnabled(!identifying && hasTab);
  mwUi->actionMergeAddFilesFromClipboard->setEnabled(!identifying && !fileNamesFromClipboard().isEmpty());
  mwUi->menuMergeModifySelectedTracks->setEnabled(!identifying && hasTab && tab->hasSelectedNotAppendedRegularTracks());
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
  auto &p = *p_func();

  p.ui->stack->setCurrentWidget(p.ui->merges->count() ? p.ui->mergesPage : p.ui->noMergesPage);
  enableMenuActions();
  enableCopyMenuActions();
}

void
Tool::toolShown() {
  MainWindow::get()->showTheseMenusOnly({ p_func()->mergeMenu });
  showMergeWidget();
}

void
Tool::retranslateUi() {
  auto &p            = *p_func();
  auto buttonToolTip = Util::Settings::get().m_uiDisableToolTips ? Q("") : App::translate("CloseButton", "Close Tab");

  p.ui->retranslateUi(this);
  p.modifyTracksSubmenu.retranslateUi();

  for (auto idx = 0, numTabs = p.ui->merges->count(); idx < numTabs; ++idx) {
    static_cast<Tab *>(p.ui->merges->widget(idx))->retranslateUi();
    auto button = Util::tabWidgetCloseTabButton(*p.ui->merges, idx);
    if (button)
      button->setToolTip(buttonToolTip);
  }

  p.languageShortcutsMenu->setTitle(QY("Set &language"));
}

Tab *
Tool::currentTab() {
  auto &p = *p_func();

  return static_cast<Tab *>(p.ui->merges->widget(p.ui->merges->currentIndex()));
}

Tab *
Tool::appendNewTab() {
  auto &p  = *p_func();
  auto tab = new Tab{this};

  connect(tab, &Tab::removeThisTab, this, &Tool::closeSendingTab);
  connect(tab, &Tab::titleChanged,  this, &Tool::tabTitleChanged);

  p.ui->merges->addTab(tab, Util::escape(tab->title(), Util::EscapeKeyboardShortcuts));
  p.ui->merges->setCurrentIndex(p.ui->merges->count() - 1);

  showMergeWidget();

  return tab;
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
    cfg.m_lastConfigDir.setPath(QFileInfo{fileName}.path());
  });

  if (MainWindow::jobTool()->addJobFile(fileName)) {
    MainWindow::get()->setStatusBarMessage(QY("The job has been added to the job queue."));
    return;
  }

  auto tab = currentTab();
  if (tab && tab->isEmpty())
    tab->deleteLater();

  appendNewTab()->load(fileName);
}

void
Tool::openFromConfig(MuxConfig const &config) {
  appendNewTab()->cloneConfig(config);
}

bool
Tool::closeTab(int index) {
  auto &p = *p_func();

  if ((0  > index) || (p.ui->merges->count() <= index))
    return false;

  auto tab = static_cast<Tab *>(p.ui->merges->widget(index));

  if (Util::Settings::get().m_warnBeforeClosingModifiedTabs && tab->hasBeenModified()) {
    MainWindow::get()->switchToTool(this);
    p.ui->merges->setCurrentIndex(index);

    auto answer = Util::MessageBox::question(this)
      ->title(QY("Close modified settings"))
      .text(QY("The multiplex settings creating \"%1\" have been modified. Do you really want to close? All changes will be lost.").arg(tab->title()))
      .buttonLabel(QMessageBox::Yes, QY("&Close settings"))
      .buttonLabel(QMessageBox::No,  QY("Cancel"))
      .exec();
    if (answer != QMessageBox::Yes)
      return false;
  }

  p.ui->merges->removeTab(index);
  delete tab;

  showMergeWidget();

  return true;
}

void
Tool::closeCurrentTab() {
  auto &p = *p_func();

  closeTab(p.ui->merges->currentIndex());
}

void
Tool::closeSendingTab() {
  auto &p  = *p_func();
  auto idx = p.ui->merges->indexOf(dynamic_cast<Tab *>(sender()));
  if (-1 != idx)
    closeTab(idx);
}

bool
Tool::closeAllTabs() {
  auto &p = *p_func();

  for (auto index = p.ui->merges->count(); index > 0; --index) {
    p.ui->merges->setCurrentIndex(index);
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
Tool::saveAllConfigsToSingleAs() {
  auto &p        = *p_func();
  auto &settings = Util::Settings::get();
  auto fileName  = Util::getSaveFileName(this, QY("Save settings file as"), settings.m_lastConfigDir.path(), Q(".mtxcfg"), QY("MKVToolNix GUI config files") + Q(" (*.mtxcfg);;") + QY("All files") + Q(" (*)"), Q("mtxcfg"));
  if (fileName.isEmpty())
    return;

  settings.m_lastConfigDir.setPath(QFileInfo{fileName}.path());
  qDebug() << "LCD" << settings.m_lastConfigDir;
  settings.save();

  p.m_singleConfigFileName = fileName;

  saveAllConfigsToSingle();
}

void
Tool::saveAllConfigsToSingle() {
  auto &p = *p_func();

  if (p.m_singleConfigFileName.isEmpty()) {
    saveAllConfigsToSingleAs();
    return;
  }

  QFile::remove(p.m_singleConfigFileName);
  auto configFile = Util::ConfigFile::create(p.m_singleConfigFileName);

  configFile->beginGroup(App::settingsBaseGroupName());
  configFile->setValue("version", Util::ConfigFile::MtxCfgVersion);
  configFile->setValue("type",    MuxConfig::settingsTypeMulti());
  configFile->endGroup();

  configFile->beginGroup("configurations");

  auto configurationCounter = 0;

  forEachTab([&configFile, &configurationCounter](Tab &tab) {
    ++configurationCounter;

    configFile->beginGroup(Q(fmt::format("{0:04d}", configurationCounter)));

    tab.saveConfigTo(*configFile);

    configFile->endGroup();
  });

  configFile->endGroup();

  configFile->save();

  MainWindow::get()->setStatusBarMessage(QY("The configuration has been saved."));
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
  auto &p                 = *p_func();
  auto const exe          = Util::Settings::get().actualMkvmergeExe();
  auto const sendingTab   = currentTab();
  auto initialMuxSettings = 0;
  auto tabs               = p.ui->merges;

  QVector<CommandLineDialog::MuxSettings> muxSettings;

  for (int tabIndex = 0; tabIndex < tabs->count(); ++tabIndex) {
    auto tab = static_cast<Tab *>(tabs->widget(tabIndex));

    if (sendingTab == tab)
      initialMuxSettings = tabIndex;

    auto options = tab->updateConfigFromControlValues().buildMkvmergeOptions().setExecutable(exe);

    muxSettings << CommandLineDialog::MuxSettings{ tab->title(), options };
  }

  CommandLineDialog{this, muxSettings, initialMuxSettings, QY("mkvmerge command line")}.exec();
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
  auto &p  = *p_func();
  auto tab = dynamic_cast<Tab *>(sender());
  auto idx = p.ui->merges->indexOf(tab);
  if (tab && (-1 != idx))
    p.ui->merges->setTabText(idx, Util::escape(tab->title(), Util::EscapeKeyboardShortcuts));
}

void
Tool::dragEnterEvent(QDragEnterEvent *event) {
  auto &p = *p_func();

  p.filesDDHandler.handle(event, false);
}

void
Tool::dragMoveEvent(QDragMoveEvent *event) {
  auto &p = *p_func();

  p.filesDDHandler.handle(event, false);
}

void
Tool::dropEvent(QDropEvent *event) {
  auto &p = *p_func();

  if (p.filesDDHandler.handle(event, true)) {
    auto fileNames    = p.filesDDHandler.fileNames();
    auto mouseButtons = event->buttons();

    QTimer::singleShot(0, this, [this, fileNames, mouseButtons]() {
      handleExternallyAddedFiles(fileNames, mouseButtons);
    });
  }
}

void
Tool::handleExternallyAddedFiles(QStringList const &fileNames,
                                 Qt::MouseButtons mouseButtons) {
  if (fileNames.isEmpty())
    return;

  Util::Settings::change([&fileNames](Util::Settings &settings) {
    settings.m_lastOpenDir.setPath(QFileInfo{fileNames[0]}.path());
  });

  auto configExt  = Q(".mtxcfg");
  auto mediaFiles = QStringList{};

  for (auto const &fileName : fileNames)
    if (fileName.endsWith(configExt))
      openConfigFile(fileName);
    else
      mediaFiles << fileName;

  if (!mediaFiles.isEmpty())
    identifyMultipleFiles(mediaFiles, mouseButtons);
}

void
Tool::handleFilesFromCommandLine(QStringList const &fileNames) {
  MainWindow::get()->switchToTool(this);
  handleExternallyAddedFiles(fileNames, Qt::NoButton);
}

void
Tool::openMultipleConfigFilesFromCommandLine(QStringList const &fileNames) {
  MainWindow::get()->switchToTool(this);
  for (auto const &fileName : fileNames)
    openConfigFile(fileName);
}

QStringList
Tool::fileNamesFromClipboard()
  const {
  // Windows Explorer copy file on mounted drive:            file:///E:/videos/file.avi
  // Windows Explorer copy file on UNC path:                 file://disky/videos/file.avi
  // Windows Explorer copy path on mounted drive in URL bar: \\disky\videos\file.avi
  // Windows Explorer copy UNC path in URL bar:              \\disky\videos\file.avi
  // Linux Krusader copy file:                               file:///home/mosu/videos/file.avi
  // Linux raw file path:                                    /home/mosu/videos/file.avi

#if defined(SYS_WINDOWS)
  QRegularExpression driveFileUriRE{Q(R"(^file:///?([a-zA-Z]:[/\\].+))")}, uncUriRE{Q(R"(^file://([[:word:]].+?/.+))")}, uncRE{Q(R"(^\\\\.+\\.)")}, driveFileRE{Q(R"(^[a-zA-Z]:[/\\].)")};
#else
  QRegularExpression fileRE{Q("^/.+")};
#endif
  QRegularExpression fileUriRE{Q("^file://(/.+)")};
  QStringList fileNames;

  for (auto const &line : App::clipboard()->text().split(Q("\n"))) {
#if defined(SYS_WINDOWS)
    if (uncRE.match(line).hasMatch() || driveFileRE.match(line).hasMatch()) {
      fileNames << line;
      continue;
    }

    if (auto match = driveFileUriRE.match(line); match.hasMatch()) {
      fileNames << match.captured(1);
      continue;
    }

    if (auto match = uncUriRE.match(line); match.hasMatch()) {
      fileNames << Q("\\\\%1").arg(match.captured(1).replace(Q("/"), Q("\\")));
      continue;
    }

#else  // defined(SYS_WINDOWS)
    if (fileRE.match(line).hasMatch()) {
      fileNames << line;
      continue;
    }
#endif  // defined(SYS_WINDOWS)

    if (auto match = fileUriRE.match(line); match.hasMatch())
      fileNames << match.captured(1);
  }

  return fileNames;
}

void
Tool::addFilesFromClipboard() {
  auto fileNames = fileNamesFromClipboard();

  if (!fileNames.isEmpty())
    identifyMultipleFiles(fileNames, Qt::NoButton);
}

void
Tool::addMergeTabIfNoneOpen() {
  auto &p = *p_func();

  if (!p.ui->merges->count())
    appendNewTab();
}

Tab *
Tool::tabForAddingOrAppending(uint64_t wantedId) {
  auto &p = *p_func();

  for (auto idx = 0, numTabs = p.ui->merges->count(); idx < numTabs; ++idx) {
    auto tab = static_cast<Tab *>(p.ui->merges->widget(idx));
    if (reinterpret_cast<uint64_t>(tab) == wantedId)
      return tab;
  }

  addMergeTabIfNoneOpen();

  return currentTab();
}

void
Tool::forEachTab(std::function<void(Tab &)> const &worker) {
  auto &p           = *p_func();
  auto currentIndex = p.ui->merges->currentIndex();

  for (auto index = 0, numTabs = p.ui->merges->count(); index < numTabs; ++index) {
    p.ui->merges->setCurrentIndex(index);
    worker(dynamic_cast<Tab &>(*p.ui->merges->widget(index)));
  }

  p.ui->merges->setCurrentIndex(currentIndex);
}

std::pair<QString, QString>
Tool::nextPreviousWindowActionTexts()
  const {
  return {
    QY("&Next multiplex settings"),
    QY("&Previous multiplex settings"),
  };
}

void
Tool::retrieveDiscInformationForPlaylists(QVector<SourceFilePtr> &sourceFiles) {
  QHash<boost::filesystem::path, mtx::bluray::disc_library::info_t> infoByBaseDir;

  for (auto const &sourceFile : sourceFiles) {
    if (!sourceFile->isPlaylist() || sourceFile->m_discLibraryInfoSelected)
      continue;

    auto baseDir = mtx::bluray::find_base_dir(mtx::fs::to_path(sourceFile->m_fileName));

    if (infoByBaseDir.contains(baseDir))
      continue;

    auto discLibrary = mtx::bluray::disc_library::locate_and_parse(mtx::fs::to_path(sourceFile->m_fileName));

    if (!discLibrary || discLibrary->m_infos_by_language.empty()) {
      infoByBaseDir.insert(baseDir, {});
      continue;
    }

    if (discLibrary->m_infos_by_language.size() == 1) {
      auto &info = discLibrary->m_infos_by_language.begin()->second;

      infoByBaseDir.insert(baseDir, info);
      sourceFile->m_discLibraryInfoToAdd = info;

      continue;
    }

    SelectDiscLibraryInformationDialog dlg{this, *discLibrary};

    if (dlg.exec() != QDialog::Accepted)
      continue;

    auto info = dlg.selectedInfo();
    if (!info)
      continue;

    infoByBaseDir.insert(baseDir, *info);
    sourceFile->m_discLibraryInfoToAdd = info;
  }
}

void
Tool::handleIdentifiedNonSourceFiles(IdentificationPack &pack) {
  for (auto const &identifiedFile : pack.m_identifiedNonSourceFiles) {
    if (identifiedFile.m_type == IdentificationPack::FileType::Chapters)
      handleIdentifiedXmlOrSimpleChapters(identifiedFile.m_fileName);

    else if (identifiedFile.m_type == IdentificationPack::FileType::Tags)
      handleIdentifiedXmlTags(identifiedFile.m_fileName);

    else if (identifiedFile.m_type == IdentificationPack::FileType::SegmentInfo)
      handleIdentifiedXmlSegmentInfo(identifiedFile.m_fileName);
  }
}

void
Tool::handleIdentifiedSourceFiles(IdentificationPack &pack) {
  auto &p = *p_func();

  auto identifiedSourceFiles = pack.sourceFiles();

  if (identifiedSourceFiles.isEmpty())
    return;

  auto tab = tabForAddingOrAppending(pack.m_tabId);

  retrieveDiscInformationForPlaylists(identifiedSourceFiles);

  if (   (pack.m_addMode == IdentificationPack::AddMode::Append)
      || (pack.m_addMode == IdentificationPack::AddMode::AddDontAsk)
      || (   (identifiedSourceFiles.count() == 1)
          && (   tab->isEmpty()
              || (pack.m_addMode == IdentificationPack::AddMode::Add)))) {
    auto addMode = pack.m_addMode == IdentificationPack::AddMode::AddDontAsk ? IdentificationPack::AddMode::Add : pack.m_addMode;
    tab->addOrAppendIdentifiedFiles(identifiedSourceFiles, pack.m_sourceFileIdx, addMode);
    return;
  }

  auto &settings                            = Util::Settings::get();

  auto isDragAndDrop                        = pack.m_addMode == IdentificationPack::AddMode::UserChoice;
  auto &policySetting                       = isDragAndDrop ? settings.m_mergeDragAndDropFilesPolicy : settings.m_mergeAddingAppendingFilesPolicy;
  auto decision                             = policySetting;
  auto fileModelIdx                         = QModelIndex{};
  auto alwaysCreateNewSettingsForVideoFiles = settings.m_mergeAlwaysCreateNewSettingsForVideoFiles;

  if (   (Util::Settings::MergeAddingAppendingFilesPolicy::Ask == decision)
      || ((pack.m_mouseButtons & Qt::RightButton)              == Qt::RightButton)) {
    auto &lastDecisionSetting = isDragAndDrop ? settings.m_mergeLastDragAndDropFilesDecision : settings.m_mergeLastAddingAppendingDecision;
    auto mode                 = isDragAndDrop ? AddingAppendingFilesDialog::DragAndDrop : AddingAppendingFilesDialog::AddSourceFiles;

    AddingAppendingFilesDialog dlg{this, *tab, mode};
    dlg.setDefaults(lastDecisionSetting, p.lastAddAppendFileNum[tab], settings.m_mergeAlwaysCreateNewSettingsForVideoFiles);
    if (!dlg.exec())
      return;

    decision                             = dlg.decision();
    alwaysCreateNewSettingsForVideoFiles = dlg.alwaysCreateNewSettingsForVideoFiles();
    fileModelIdx                         = tab->fileModelIndexForFileNum(dlg.fileNum());

    lastDecisionSetting                  = decision;
    p.lastAddAppendFileNum[tab]          = dlg.fileNum();

    if (dlg.alwaysUseThisDecision()) {
      policySetting                                        = decision;
      settings.m_mergeAlwaysCreateNewSettingsForVideoFiles = alwaysCreateNewSettingsForVideoFiles;
    }

    settings.save();
  }

  if (alwaysCreateNewSettingsForVideoFiles) {
    auto tempIdentifiedSourceFiles = std::move(identifiedSourceFiles);
    identifiedSourceFiles.clear();

    for (auto const &identifiedSourceFile : tempIdentifiedSourceFiles) {
      if (!identifiedSourceFile->hasVideoTrack()) {
        identifiedSourceFiles << identifiedSourceFile;
        continue;
      }

      auto tabToUse = tab->isEmpty() ? tab : appendNewTab();
      tabToUse->addOrAppendIdentifiedFiles({ identifiedSourceFile }, {}, IdentificationPack::AddMode::Add);
    }

    if (identifiedSourceFiles.isEmpty())
      return;
  }

  if (Util::Settings::MergeAddingAppendingFilesPolicy::AddAdditionalParts == decision)
    tab->addIdentifiedFilesAsAdditionalParts(identifiedSourceFiles, fileModelIdx);

  else if (Util::Settings::MergeAddingAppendingFilesPolicy::AddToNew == decision) {
    auto newTab = appendNewTab();
    newTab->addOrAppendIdentifiedFiles(identifiedSourceFiles, {}, IdentificationPack::AddMode::Add);

  } else if (Util::Settings::MergeAddingAppendingFilesPolicy::AddEachToNew == decision) {
    if (tab->isEmpty())
      tab->addOrAppendIdentifiedFiles({ identifiedSourceFiles.takeFirst() }, {}, IdentificationPack::AddMode::Add);

    for (auto const &identifiedSourceFile : identifiedSourceFiles) {
      auto newTab = appendNewTab();
      newTab->addOrAppendIdentifiedFiles({ identifiedSourceFile }, {}, IdentificationPack::AddMode::Add);
    }

  } else
    tab->addOrAppendIdentifiedFiles(identifiedSourceFiles, fileModelIdx, Util::Settings::MergeAddingAppendingFilesPolicy::Append == decision ? IdentificationPack::AddMode::Append : IdentificationPack::AddMode::Add);
}

void
Tool::handleIdentifiedFiles(IdentificationPack identifiedFiles) {
  addMergeTabIfNoneOpen();

  handleIdentifiedSourceFiles(identifiedFiles);
  handleIdentifiedNonSourceFiles(identifiedFiles);
}

std::optional<Util::Settings::MergeAddingDirectoriesPolicy>
Tool::determineAddingDirectoriesPolicy() {
  auto &settings = Util::Settings::get();
  auto policy    = settings.m_mergeDragAndDropDirectoriesPolicy;

  if (policy != Util::Settings::MergeAddingDirectoriesPolicy::Ask)
    return policy;

  AddingDirectoriesDialog dlg{this, policy};

  if (!dlg.exec())
    return {};

  policy = dlg.decision();

  if (dlg.alwaysUseThisDecision()) {
    settings.m_mergeDragAndDropDirectoriesPolicy = policy;
    settings.save();
  }

  return policy;
}

void
Tool::addFileIdentificationPack(QStringList const &fileNames,
                                IdentificationPack::AddMode addMode,
                                Qt::MouseButtons mouseButtons) {
  auto &p = *p_func();

  if (fileNames.isEmpty())
    return;

  IdentificationPack pack;
  pack.m_tabId        = reinterpret_cast<uint64_t>(currentTab());
  pack.m_fileNames    = fileNames;
  pack.m_addMode      = addMode;
  pack.m_mouseButtons = mouseButtons;

  qDebug() << "tabId" << pack.m_tabId << pack.m_fileNames;

  p.identifier->worker().addPackToIdentify(pack);
}

void
Tool::identifyMultipleFiles(QStringList const &fileNamesToIdentify,
                            Qt::MouseButtons mouseButtons) {
  auto resolvedDirs = resolveDirectoriesToContainedFilesAndDirectories(fileNamesToIdentify);

  if (resolvedDirs.m_files.isEmpty() && resolvedDirs.m_directories.isEmpty())
    return;

  if (resolvedDirs.m_directories.isEmpty()) {
    addFileIdentificationPack(resolvedDirs.m_files, IdentificationPack::AddMode::UserChoice, mouseButtons);
    return;
  }

  auto policyOpt = determineAddingDirectoriesPolicy();

  if (!policyOpt)
    return;

  auto policy = *policyOpt;

  if (policy == Util::Settings::MergeAddingDirectoriesPolicy::Flat) {
    auto fileNames = resolvedDirs.m_files;

    for (auto const &dir : resolvedDirs.m_directories)
      fileNames += dir.second;

    addFileIdentificationPack(fileNames, IdentificationPack::AddMode::UserChoice, mouseButtons);
    return;
  }

  if (!currentTab())
    appendNewTab();

  addFileIdentificationPack(resolvedDirs.m_files, IdentificationPack::AddMode::UserChoice, mouseButtons);

  // Util::Settings::MergeAddingDirectoriesPolicy::AddEachDirectoryToNew
  for (auto const &dir : resolvedDirs.m_directories) {
    appendNewTab();
    addFileIdentificationPack(dir.second, IdentificationPack::AddMode::AddDontAsk, mouseButtons);
  }
}

void
Tool::fileIdentificationStarted(unsigned int numberOfQueuedFiles) {
  auto &p = *p_func();

  qDebug() << "Tool::fileIdentificationStarted with" << numberOfQueuedFiles;

  p.ui->overlordWidget->setEnabled(false);

  enableMenuActions();

  if (!p.identifyingFilesDialog)
    p.identifyingFilesDialog = new QProgressDialog{ QY("Identifying files"), QY("Cancel"), 0, static_cast<int>(numberOfQueuedFiles), this };
  p.identifyingFilesDialog->setWindowTitle(QY("Identifying files"  ));
  p.identifyingFilesDialog->show();

  connect(p.identifyingFilesDialog, &QProgressDialog::canceled, p.identifier, &FileIdentificationThread::abortIdentification);
}

void
Tool::fileIdentificationFinished() {
  auto &p = *p_func();

  qDebug() << "Tool::fileIdentificationFinished";

  delete p.identifyingFilesDialog;
  p.identifyingFilesDialog = nullptr;

  p.ui->overlordWidget->setEnabled(true);

  enableMenuActions();
}

void
Tool::updateFileIdentificationProgress(unsigned int numberOfIdentifiedFiles,
                                       unsigned int numberOfQueuedFiles) {
  auto &p = *p_func();

  qDebug() << "Tool::updateFileIdentificationProgress with " << numberOfIdentifiedFiles << "/" << numberOfQueuedFiles;

  if (p.identifyingFilesDialog) {
    p.identifyingFilesDialog->setValue(static_cast<int>(numberOfIdentifiedFiles));
    p.identifyingFilesDialog->setMaximum(static_cast<int>(numberOfQueuedFiles));
  }
}

void
Tool::handleIdentifiedXmlOrSimpleChapters(QString const &fileName) {
  Util::MessageBox::warning(this)
    ->title(QY("Adding chapter files"))
    .text(Q("%1 %2 %3 %4")
          .arg(QY("The file '%1' contains chapters.").arg(fileName))
          .arg(QY("These aren't treated like other source files in MKVToolNix."))
          .arg(QY("Instead such a file must be set via the 'chapter file' option on the 'output' tab."))
          .arg(QY("The GUI will enter the dropped file's file name into that control replacing any file name which might have been set earlier.")))
    .onlyOnce(Q("mergeChaptersDropped"))
    .exec();

  currentTab()->setChaptersFileName(fileName);
}

void
Tool::handleIdentifiedXmlSegmentInfo(QString const &fileName) {
  Util::MessageBox::warning(this)
    ->title(QY("Adding segment info files"))
    .text(Q("%1 %2 %3 %4")
          .arg(QY("The file '%1' contains segment information.").arg(fileName))
          .arg(QY("These aren't treated like other source files in MKVToolNix."))
          .arg(QY("Instead such a file must be set via the 'segment info' option on the 'output' tab."))
          .arg(QY("The GUI will enter the dropped file's file name into that control replacing any file name which might have been set earlier.")))
    .onlyOnce(Q("mergeSegmentInfoDropped"))
    .exec();

  currentTab()->setSegmentInfoFileName(fileName);
}

void
Tool::handleIdentifiedXmlTags(QString const &fileName) {
  Util::MessageBox::warning(this)
    ->title(QY("Adding tag files"))
    .text(Q("%1 %2 %3 %4")
          .arg(QY("The file '%1' contains tags.").arg(fileName))
          .arg(QY("These aren't treated like other source files in MKVToolNix."))
          .arg(QY("Instead such a file must be set via the 'global tags' option on the 'output' tab."))
          .arg(QY("The GUI will enter the dropped file's file name into that control replacing any file name which might have been set earlier.")))
    .onlyOnce(Q("mergeTagsDropped"))
    .exec();

  currentTab()->setTagsFileName(fileName);
}

void
Tool::showFileIdentificationError(QString const &errorTitle,
                                 QString const &errorText) {
  auto &p  = *p_func();
  auto dlg = Util::MessageBox::critical(this);

  if (!p.identifier->isEmpty()) {
    dlg->buttons(QMessageBox::Ok | QMessageBox::Cancel);
    dlg->buttonLabel(QMessageBox::Ok, QY("&Continue identification"));
  }

  auto result = dlg->title(errorTitle).text(errorText).exec();

  if (p.identifier->isEmpty() || (result == QMessageBox::Ok))
    p.identifier->continueIdentification();
  else
    p.identifier->abortIdentification();
}

void
Tool::selectScanPlaylistPolicy(SourceFilePtr const &sourceFile,
                               QFileInfoList const &files) {
  auto &p = *p_func();

  AskScanForPlaylistsDialog dialog{this};

  if (dialog.ask(*sourceFile, files.count() - 1))
    p.identifier->continueByScanningPlaylists(files);

  else {
    p.identifier->worker().addIdentifiedFile(sourceFile);
    p.identifier->continueIdentification();
  }
}

void
Tool::hideScanningDirectoryDialog() {
  auto &p = *p_func();

  delete p.scanningDirectoryDialog;
  p.scanningDirectoryDialog = nullptr;

  if (p.identifyingFilesDialog)
    p.identifyingFilesDialog->show();
}

void
Tool::showScanningPlaylistDialog(int numFilesToScan) {
  auto &p      = *p_func();
  auto &worker = p.identifier->worker();

  if (p.identifyingFilesDialog)
    p.identifyingFilesDialog->hide();

  if (!p.scanningDirectoryDialog)
    p.scanningDirectoryDialog = new QProgressDialog{ QY("Scanning directory"), QY("Cancel"), 0, numFilesToScan, this };

  connect(&worker,                   &FileIdentificationWorker::playlistScanProgressChanged, p.scanningDirectoryDialog, &QProgressDialog::setValue);
  connect(p.scanningDirectoryDialog, &QProgressDialog::canceled,                             p.identifier,              &FileIdentificationThread::abortPlaylistScan);

  p.scanningDirectoryDialog->setWindowTitle(QY("Scanning directory"));
  p.scanningDirectoryDialog->show();
}

void
Tool::selectPlaylistToAdd(QVector<SourceFilePtr> const &identifiedPlaylists) {
  auto &p = *p_func();

  if (identifiedPlaylists.isEmpty())
    return;

  auto discLibrary = mtx::bluray::disc_library::locate_and_parse(mtx::fs::to_path(identifiedPlaylists[0]->m_fileName));

  if ((identifiedPlaylists.size() == 1) && (!discLibrary || (discLibrary->m_infos_by_language.size() <= 1))) {
    if (discLibrary && (discLibrary->m_infos_by_language.size() == 1))
      identifiedPlaylists[0]->m_discLibraryInfoToAdd = discLibrary->m_infos_by_language.begin()->second;

    p.identifier->worker().addIdentifiedFile(identifiedPlaylists[0]);

    p.identifier->continueIdentification();

    return;
  }

  auto playlists = SelectPlaylistDialog{this, identifiedPlaylists, discLibrary}.select();

  for (auto const &playlist : playlists)
    p.identifier->worker().addIdentifiedFile(playlist);

  if (p.identifyingFilesDialog)
    p.identifyingFilesDialog->show();

  p.identifier->continueIdentification();
}

FileIdentificationWorker &
Tool::identifier() {
  return MainWindow::mergeTool()->p_func()->identifier->worker();
}

void
Tool::setupHorizontalScrollAreaInputLayout() {
  forEachTab([](Tab &tab) { tab.setupHorizontalScrollAreaInputLayout(); });
}

void
Tool::setupHorizontalTwoColumnsInputLayout() {
  forEachTab([](Tab &tab) { tab.setupHorizontalTwoColumnsInputLayout(); });
}

void
Tool::setupVerticalTabWidgetInputLayout() {
  forEachTab([](Tab &tab) { tab.setupVerticalTabWidgetInputLayout(); });
}

void
Tool::toggleTrackFlag() {
  auto action = dynamic_cast<QAction *>(sender());
  auto tab    = currentTab();

  if (action && tab)
    tab->toggleSpecificTrackFlag(action->data().toUInt());
}

void
Tool::changeTrackLanguage(QString const &formattedLanguage,
                          QString const &trackName) {
  auto tab = currentTab();

  if (tab)
    tab->changeTrackLanguage(formattedLanguage, trackName);
}

}
