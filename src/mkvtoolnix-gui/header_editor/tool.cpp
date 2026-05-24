#include "common/common_pch.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenu>
#include <QMimeData>

#include "common/qt.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/forms/header_editor/tool.h"
#include "mkvtoolnix-gui/forms/main_window/main_window.h"
#include "mkvtoolnix-gui/header_editor/tab.h"
#include "mkvtoolnix-gui/header_editor/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/merge/mux_config.h"
#include "mkvtoolnix-gui/util/file_dialog.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/string.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx::gui::HeaderEditor {

using namespace mtx::gui;

Tool::Tool(QWidget *parent,
           QMenu *headerEditorMenu)
  : ToolBase{parent}
  , ui{new Ui::Tool}
  , m_headerEditorMenu{headerEditorMenu}
  , m_filesDDHandler{Util::FilesDragDropHandler::Mode::Remember}
{
  // Setup UI controls.
  ui->setupUi(this);

  MainWindow::get()->registerSubWindowWidget(*this, *ui->editors);
}

Tool::~Tool() {
}

void
Tool::setupUi() {
  setupModifyTracksMenu();

  Util::setupTabWidgetHeaders(*ui->editors);

  showHeaderEditorsWidget();

  retranslateUi();

  connect(ui->editors, &QTabWidget::tabCloseRequested, this, &Tool::closeTab);
}

void
Tool::setupActions() {
  auto mw   = MainWindow::get();
  auto mwUi = MainWindow::getUi();
  auto &mts = m_modifyTracksSubmenu;

  connect(mwUi->actionHeaderEditorOpen,     &QAction::triggered,                                 this, &Tool::selectFileToOpen);
  connect(mwUi->actionHeaderEditorSave,     &QAction::triggered,                                 this, &Tool::save);
  connect(mwUi->actionHeaderEditorSaveAll,  &QAction::triggered,                                 this, &Tool::saveAllTabs);
  connect(mwUi->actionHeaderEditorValidate, &QAction::triggered,                                 this, &Tool::validate);
  connect(mwUi->actionHeaderEditorReload,   &QAction::triggered,                                 this, &Tool::reload);
  connect(mwUi->actionHeaderEditorClose,    &QAction::triggered,                                 this, &Tool::closeCurrentTab);
  connect(mwUi->actionHeaderEditorCloseAll, &QAction::triggered,                                 this, &Tool::closeAllTabs);

  connect(mts.m_toggleTrackEnabledFlag,     &QAction::triggered,                                 this, &Tool::toggleTrackFlag);
  connect(mts.m_toggleDefaultTrackFlag,     &QAction::triggered,                                 this, &Tool::toggleTrackFlag);
  connect(mts.m_toggleForcedDisplayFlag,    &QAction::triggered,                                 this, &Tool::toggleTrackFlag);
  connect(mts.m_toggleCommentaryFlag,       &QAction::triggered,                                 this, &Tool::toggleTrackFlag);
  connect(mts.m_toggleOriginalFlag,         &QAction::triggered,                                 this, &Tool::toggleTrackFlag);
  connect(mts.m_toggleHearingImpairedFlag,  &QAction::triggered,                                 this, &Tool::toggleTrackFlag);
  connect(mts.m_toggleVisualImpairedFlag,   &QAction::triggered,                                 this, &Tool::toggleTrackFlag);
  connect(mts.m_toggleTextDescriptionsFlag, &QAction::triggered,                                 this, &Tool::toggleTrackFlag);

  connect(&mts,                             &Util::ModifyTracksSubmenu::languageChangeRequested, this, &Tool::changeTrackLanguage);

  connect(ui->openFileButton,               &QPushButton::clicked,                               this, &Tool::selectFileToOpen);

  connect(m_headerEditorMenu,               &QMenu::aboutToShow,                                 this, &Tool::enableMenuActions);
  connect(mw,                               &MainWindow::preferencesChanged,                     this, &Tool::applyPreferences);

  connect(App::instance(),                  &App::editingHeadersRequested,                       this, &Tool::openFilesFromCommandLine);
}

void
Tool::setupModifyTracksMenu() {
  auto mwUi               = MainWindow::getUi();
  m_languageShortcutsMenu = new QMenu{this};

  mwUi->menuHeaderEditorModifyTrack->addMenu(m_languageShortcutsMenu);
  mwUi->menuHeaderEditorModifyTrack->addSeparator();

  m_modifyTracksSubmenu.setupTrack(*mwUi->menuHeaderEditorModifyTrack);
  m_modifyTracksSubmenu.setupLanguage(*m_languageShortcutsMenu);
}

void
Tool::applyPreferences() {
  Util::setupTabWidgetHeaders(*ui->editors);;
  m_modifyTracksSubmenu.setupLanguage(*m_languageShortcutsMenu);
  retranslateUi();
}

void
Tool::showHeaderEditorsWidget() {
  auto hasTabs = !!ui->editors->count();

  ui->stack->setCurrentWidget(hasTabs ? ui->editorsPage : ui->noFilesPage);

  enableMenuActions();
}

void
Tool::enableMenuActions() {
  auto hasTabs = !!ui->editors->count();
  auto mwUi    = MainWindow::getUi();
  auto tab     = currentTab();

  mwUi->actionHeaderEditorSave->setEnabled(hasTabs);
  mwUi->actionHeaderEditorReload->setEnabled(hasTabs);
  mwUi->actionHeaderEditorValidate->setEnabled(hasTabs);
  mwUi->actionHeaderEditorClose->setEnabled(hasTabs);
  mwUi->menuHeaderEditorAll->setEnabled(hasTabs);
  mwUi->actionHeaderEditorSaveAll->setEnabled(hasTabs);
  mwUi->actionHeaderEditorCloseAll->setEnabled(hasTabs);
  mwUi->menuHeaderEditorAll->setEnabled(hasTabs);
  mwUi->actionHeaderEditorSaveAll->setEnabled(hasTabs);
  mwUi->actionHeaderEditorCloseAll->setEnabled(hasTabs);
  mwUi->menuHeaderEditorModifyTrack->setEnabled(tab && tab->isTrackSelected());
}

void
Tool::toolShown() {
  MainWindow::get()->showTheseMenusOnly({ m_headerEditorMenu });
  showHeaderEditorsWidget();
}

void
Tool::retranslateUi() {
  auto buttonToolTip = Util::Settings::get().m_uiDisableToolTips ? Q("") : App::translate("CloseButton", "Close Tab");

  ui->retranslateUi(this);

  m_modifyTracksSubmenu.retranslateUi();
  m_languageShortcutsMenu->setTitle(QY("Set &language"));

  for (auto idx = 0, numTabs = ui->editors->count(); idx < numTabs; ++idx) {
    static_cast<Tab *>(ui->editors->widget(idx))->retranslateUi();
    auto button = Util::tabWidgetCloseTabButton(*ui->editors, idx);
    if (button)
      button->setToolTip(buttonToolTip);
  }
}

void
Tool::dragEnterEvent(QDragEnterEvent *event) {
  m_filesDDHandler.handle(event, false);
}

void
Tool::dropEvent(QDropEvent *event) {
  if (m_filesDDHandler.handle(event, true))
    openFiles(m_filesDDHandler.fileNames());
}

void
Tool::openFiles(QStringList const &fileNames) {
  for (auto const &file : fileNames)
    openFile(file);
}

void
Tool::openFilesFromCommandLine(QStringList const &fileNames) {
  MainWindow::get()->switchToTool(this);
  openFiles(fileNames);
}

void
Tool::openFile(QString const &fileName) {
  auto &settings = Util::Settings::get();
  settings.m_lastOpenDir.setPath(QFileInfo{fileName}.path());
  settings.save();

  auto tab = new Tab{this, fileName};

  connect(tab, &Tab::removeThisTab, this, &Tool::closeSendingTab);
  ui->editors->addTab(tab, Util::escape(tab->title(), Util::EscapeKeyboardShortcuts));

  showHeaderEditorsWidget();

  ui->editors->setCurrentIndex(ui->editors->count() - 1);

  tab->load();
}

void
Tool::selectFileToOpen() {
  auto fileNames = Util::getOpenFileNames(this, QY("Open Matroska or WebM files"), Util::Settings::get().lastOpenDirPath(), QY("Matroska and WebM files") + Q(" (*.mkv *.mka *.mks *.mk3d *.webm);;") + QY("All files") + Q(" (*)"));
  if (fileNames.isEmpty())
    return;

  MainWindow::get()->setStatusBarMessage(QNY("Opening %1 file in the header editor…", "Opening %1 files in the header editor…", fileNames.count()).arg(fileNames.count()));

  for (auto const &fileName : fileNames)
    openFile(fileName);
}

void
Tool::save() {
  auto tab = currentTab();
  if (tab)
    tab->save();
}

void
Tool::reload() {
  auto tab = currentTab();
  if (tab && tab->isClosingOrReloadingOkIfModified(Tab::ModifiedConfirmationMode::Reloading))
    tab->load();
}

void
Tool::validate() {
  auto tab = currentTab();
  if (tab)
    tab->validate();
}


bool
Tool::closeTab(int index) {
  if ((0  > index) || (ui->editors->count() <= index))
    return false;

  auto tab = static_cast<Tab *>(ui->editors->widget(index));
  if (!tab->isClosingOrReloadingOkIfModified(Tab::ModifiedConfirmationMode::Closing))
    return false;

  ui->editors->removeTab(index);
  delete tab;

  showHeaderEditorsWidget();

  return true;
}

void
Tool::closeCurrentTab() {
  closeTab(ui->editors->currentIndex());
}

void
Tool::closeSendingTab() {
  auto toRemove = static_cast<Tab *>(sender());

  for (auto idx = 0, count = ui->editors->count(); idx < count; ++idx) {
    auto tab = static_cast<Tab *>(ui->editors->widget(idx));
    if (tab == toRemove) {
      closeTab(idx);
      return;
    }
  }
}

void
Tool::saveAllTabs() {
  forEachTab([](auto &tab) { tab.save(); });
}

bool
Tool::closeAllTabs() {
  for (auto index = ui->editors->count(); index > 0; --index)
    if (!closeTab(index - 1))
      return false;

  return true;
}

Tab *
Tool::currentTab() {
  return static_cast<Tab *>(ui->editors->widget(ui->editors->currentIndex()));
}

void
Tool::forEachTab(std::function<void(Tab &)> const &worker) {
  auto currentIndex = ui->editors->currentIndex();

  for (auto index = 0, numTabs = ui->editors->count(); index < numTabs; ++index) {
    ui->editors->setCurrentIndex(index);
    worker(dynamic_cast<Tab &>(*ui->editors->widget(index)));
  }

  ui->editors->setCurrentIndex(currentIndex);
}

void
Tool::showTab(Tab &tab) {
  ui->editors->setCurrentWidget(&tab);
}

std::pair<QString, QString>
Tool::nextPreviousWindowActionTexts()
  const {
  return {
    QY("&Next header editor tab"),
    QY("&Previous header editor tab"),
  };
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
