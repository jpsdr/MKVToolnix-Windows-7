#include "common/common_pch.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMenu>
#include <QMessageBox>

#include "common/qt.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/forms/info/tool.h"
#include "mkvtoolnix-gui/forms/main_window/main_window.h"
#include "mkvtoolnix-gui/info/tab.h"
#include "mkvtoolnix-gui/info/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/util/file_dialog.h"
#include "mkvtoolnix-gui/util/message_box.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/string.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx::gui::Info {

using namespace mtx::gui;

Tool::Tool(QWidget *parent,
           QMenu *infoMenu)
  : ToolBase{parent}
  , ui{new Ui::Tool}
  , m_infoMenu{infoMenu}
  , m_filesDDHandler{Util::FilesDragDropHandler::Mode::Remember}
{
  // Setup UI controls.
  ui->setupUi(this);

  MainWindow::get()->registerSubWindowWidget(*this, *ui->infos);
}

Tool::~Tool() {
}

void
Tool::setupUi() {
  Util::setupTabWidgetHeaders(*ui->infos);

  showInfoWidget();

  retranslateUi();
}

void
Tool::setupActions() {
  auto mw   = MainWindow::get();
  auto mwUi = MainWindow::getUi();

  connect(m_infoMenu,               &QMenu::aboutToShow,             this, &Tool::enableMenuActions);

  connect(mwUi->actionInfoSave,     &QAction::triggered,             this, &Tool::saveCurrentTab);
  connect(mwUi->actionInfoOpen,     &QAction::triggered,             this, &Tool::selectAndOpenFile);
  connect(mwUi->actionInfoClose,    &QAction::triggered,             this, &Tool::closeCurrentTab);
  connect(mwUi->actionInfoCloseAll, &QAction::triggered,             this, &Tool::closeAllTabs);

  connect(ui->infos,                &QTabWidget::tabCloseRequested,  this, &Tool::closeTab);
  connect(ui->openFileButton,       &QPushButton::clicked,           this, &Tool::selectAndOpenFile);

  connect(mw,                       &MainWindow::preferencesChanged, [this]() { Util::setupTabWidgetHeaders(*ui->infos); });
  connect(mw,                       &MainWindow::preferencesChanged, this, &Tool::retranslateUi);

  connect(App::instance(),          &App::runningInfoOnRequested,    this, &Tool::openMultipleFilesFromCommandLine);
}

void
Tool::enableMenuActions() {
  auto mwUi   = MainWindow::getUi();
  auto tab    = currentTab();
  auto hasTab = tab && tab->isEnabled();

  mwUi->actionInfoSave->setEnabled(hasTab);
  mwUi->actionInfoClose->setEnabled(hasTab);
  mwUi->actionInfoCloseAll->setEnabled(hasTab);
}

void
Tool::showInfoWidget() {
  ui->stack->setCurrentWidget(ui->infos->count() ? ui->infosPage : ui->noInfosPage);
  enableMenuActions();
}

void
Tool::toolShown() {
  MainWindow::get()->showTheseMenusOnly({ m_infoMenu });
  showInfoWidget();
}

void
Tool::retranslateUi() {
  auto buttonToolTip = Util::Settings::get().m_uiDisableToolTips ? Q("") : App::translate("CloseButton", "Close Tab");

  ui->retranslateUi(this);

  for (auto idx = 0, numTabs = ui->infos->count(); idx < numTabs; ++idx) {
    static_cast<Tab *>(ui->infos->widget(idx))->retranslateUi();
    auto button = Util::tabWidgetCloseTabButton(*ui->infos, idx);
    if (button)
      button->setToolTip(buttonToolTip);
  }
}

Tab *
Tool::currentTab() {
  return static_cast<Tab *>(ui->infos->widget(ui->infos->currentIndex()));
}

Tab *
Tool::appendTab(Tab *tab) {
  connect(tab, &Tab::removeThisTab, this, &Tool::closeSendingTab);
  connect(tab, &Tab::titleChanged,  this, &Tool::tabTitleChanged);

  ui->infos->addTab(tab, Util::escape(tab->title(), Util::EscapeKeyboardShortcuts));
  ui->infos->setCurrentIndex(ui->infos->count() - 1);

  showInfoWidget();

  return tab;
}

void
Tool::selectAndOpenFile() {
  auto fileNames = Util::getOpenFileNames(this, QY("Open Matroska or WebM files"), Util::Settings::get().lastOpenDirPath(), QY("Matroska and WebM files") + Q(" (*.mkv *.mka *.mks *.mk3d *.webm);;") + QY("All files") + Q(" (*)"));

  for (auto const &fileName : fileNames)
    openFile(fileName);
}

void
Tool::openFile(QString const &fileName) {
  Util::Settings::change([&fileName](Util::Settings &settings) {
    settings.m_lastOpenDir.setPath(QFileInfo{fileName}.path());
  });

  appendTab(new Tab{this})
    ->load(fileName);
}

bool
Tool::closeTab(int index) {
  if ((0  > index) || (ui->infos->count() <= index))
    return false;

  auto tab = static_cast<Tab *>(ui->infos->widget(index));

  ui->infos->removeTab(index);
  delete tab;

  showInfoWidget();

  return true;
}

void
Tool::closeCurrentTab() {
  closeTab(ui->infos->currentIndex());
}

void
Tool::closeSendingTab() {
  auto idx = ui->infos->indexOf(dynamic_cast<Tab *>(sender()));
  if (-1 != idx)
    closeTab(idx);
}

bool
Tool::closeAllTabs() {
  for (auto index = ui->infos->count(); index > 0; --index) {
    ui->infos->setCurrentIndex(index);
    if (!closeTab(index - 1))
      return false;
  }

  return true;
}

void
Tool::saveCurrentTab() {
  auto tab = currentTab();
  if (tab)
    tab->save();
}

void
Tool::tabTitleChanged() {
  auto tab = dynamic_cast<Tab *>(sender());
  auto idx = ui->infos->indexOf(tab);
  if (tab && (-1 != idx))
    ui->infos->setTabText(idx, Util::escape(tab->title(), Util::EscapeKeyboardShortcuts));
}

void
Tool::dragEnterEvent(QDragEnterEvent *event) {
  m_filesDDHandler.handle(event, false);
}

void
Tool::dropEvent(QDropEvent *event) {
  if (m_filesDDHandler.handle(event, true))
    filesDropped(m_filesDDHandler.fileNames(), event->buttons());
}

void
Tool::filesDropped(QStringList const &fileNames,
                   Qt::MouseButtons /* mouseButtons */) {
  for (auto const &fileName : fileNames)
    openFile(fileName);
}

void
Tool::openMultipleFilesFromCommandLine(QStringList const &fileNames) {
  MainWindow::get()->switchToTool(this);
  for (auto const &fileName : fileNames)
    openFile(fileName);
}

std::pair<QString, QString>
Tool::nextPreviousWindowActionTexts()
  const {
  return {
    QY("&Next info tab"),
    QY("&Previous info tab"),
  };
}

}
