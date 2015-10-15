#include "common/common_pch.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>

#include "common/qt.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/forms/chapter_editor/tool.h"
#include "mkvtoolnix-gui/forms/main_window/main_window.h"
#include "mkvtoolnix-gui/chapter_editor/tab.h"
#include "mkvtoolnix-gui/chapter_editor/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/merge/mux_config.h"
#include "mkvtoolnix-gui/util/file_dialog.h"
#include "mkvtoolnix-gui/util/message_box.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx { namespace gui { namespace ChapterEditor {

using namespace mtx::gui;

Tool::Tool(QWidget *parent,
           QMenu *chapterEditorMenu)
  : ToolBase{parent}
  , ui{new Ui::Tool}
  , m_chapterEditorMenu{chapterEditorMenu}
  , m_filesDDHandler{Util::FilesDragDropHandler::Mode::Remember}
{
  // Setup UI controls.
  ui->setupUi(this);
}

Tool::~Tool() {
}

void
Tool::setupUi() {
  setupTabPositions();

  showChapterEditorsWidget();

  retranslateUi();

  connect(ui->editors, &QTabWidget::tabCloseRequested, this, &Tool::closeTab);
}

void
Tool::setupActions() {
  auto mw   = MainWindow::get();
  auto mwUi = MainWindow::getUi();

  connect(mwUi->actionChapterEditorNew,            &QAction::triggered,             this, &Tool::newFile);
  connect(mwUi->actionChapterEditorOpen,           &QAction::triggered,             this, &Tool::selectFileToOpen);
  connect(mwUi->actionChapterEditorSave,           &QAction::triggered,             this, &Tool::save);
  connect(mwUi->actionChapterEditorSaveAsXml,      &QAction::triggered,             this, &Tool::saveAsXml);
  connect(mwUi->actionChapterEditorSaveToMatroska, &QAction::triggered,             this, &Tool::saveToMatroska);
  connect(mwUi->actionChapterEditorReload,         &QAction::triggered,             this, &Tool::reload);
  connect(mwUi->actionChapterEditorClose,          &QAction::triggered,             this, &Tool::closeCurrentTab);

  connect(ui->newFileButton,                       &QPushButton::clicked,           this, &Tool::newFile);
  connect(ui->openFileButton,                      &QPushButton::clicked,           this, &Tool::selectFileToOpen);

  connect(m_chapterEditorMenu,                     &QMenu::aboutToShow,             this, &Tool::enableMenuActions);
  connect(mw,                                      &MainWindow::preferencesChanged, this, &Tool::setupTabPositions);
  connect(mw,                                      &MainWindow::preferencesChanged, this, &Tool::retranslateUi);

  connect(App::instance(),                         &App::editingChaptersRequested,  this, &Tool::openFilesFromCommandLine);
}

void
Tool::showChapterEditorsWidget() {
  ui->stack->setCurrentWidget(ui->editors->count() ? ui->editorsPage : ui->noFilesPage);
  enableMenuActions();
}

void
Tool::enableMenuActions() {
  auto mwUi        = MainWindow::getUi();
  auto tab         = currentTab();
  auto hasFileName = tab && !tab->fileName().isEmpty();
  auto hasElements = tab && tab->hasChapters();
  auto tabEnabled  = tab && tab->areWidgetsEnabled();

  mwUi->actionChapterEditorSave->setEnabled(hasFileName && hasElements && tabEnabled);
  mwUi->actionChapterEditorSaveAsXml->setEnabled(!!tab && hasElements && tabEnabled);
  mwUi->actionChapterEditorSaveToMatroska->setEnabled(!!tab && hasElements && tabEnabled);
  mwUi->actionChapterEditorReload->setEnabled(hasFileName && tabEnabled);
  mwUi->actionChapterEditorClose->setEnabled(!!tab);
}

void
Tool::toolShown() {
  MainWindow::get()->showTheseMenusOnly({ m_chapterEditorMenu });
  showChapterEditorsWidget();
}

void
Tool::retranslateUi() {
  ui->retranslateUi(this);
  for (auto idx = 0, numTabs = ui->editors->count(); idx < numTabs; ++idx) {
    static_cast<Tab *>(ui->editors->widget(idx))->retranslateUi();
    auto button = Util::tabWidgetCloseTabButton(*ui->editors, idx);
    if (button)
      button->setToolTip(App::translate("CloseButton", "Close Tab"));
  }
}

Tab *
Tool::appendTab(Tab *tab) {
  connect(tab, &Tab::removeThisTab,          this, &Tool::closeSendingTab);
  connect(tab, &Tab::titleChanged,           this, &Tool::tabTitleChanged);
  connect(tab, &Tab::numberOfEntriesChanged, this, &Tool::enableMenuActions);

  ui->editors->addTab(tab, tab->title());

  showChapterEditorsWidget();

  ui->editors->setCurrentIndex(ui->editors->count() - 1);

  return tab;
}

void
Tool::newFile() {
  appendTab(new Tab{this})
    ->newFile();
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
Tool::openFile(QString const &fileName) {
  auto &settings = Util::Settings::get();
  settings.m_lastOpenDir = QFileInfo{fileName}.path();
  settings.save();

  appendTab(new Tab{this, fileName})
   ->load();
}

void
Tool::openFiles(QStringList const &fileNames) {
  for (auto const &fileName : fileNames)
    openFile(fileName);
}

void
Tool::openFilesFromCommandLine(QStringList const &fileNames) {
  MainWindow::get()->switchToTool(this);
  openFiles(fileNames);
}

void
Tool::selectFileToOpen() {
  auto fileNames = Util::getOpenFileNames(this, QY("Open files in chapter editor"), Util::Settings::get().m_lastOpenDir.path(),
                                          QY("Supported file types")           + Q(" (*.mpls *.mkv *.mka *.mks *.mk3d *.txt *.xml);;") +
                                          QY("Matroska files")                 + Q(" (*.mkv *.mka *.mks *.mk3d);;") +
                                          QY("Blu-ray playlist files")         + Q(" (*.mpls);;") +
                                          QY("XML chapter files")              + Q(" (*.xml);;") +
                                          QY("Simple OGM-style chapter files") + Q(" (*.txt);;") +
                                          QY("All files")                      + Q(" (*)"));
  if (fileNames.isEmpty())
    return;

  MainWindow::get()->setStatusBarMessage(QNY("Opening %1 file in the chapter editor…", "Opening %1 files in the chapter editor…", fileNames.count()).arg(fileNames.count()));

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
Tool::saveAsXml() {
  auto tab = currentTab();
  if (tab) {
    tab->saveAsXml();
    enableMenuActions();
  }
}

void
Tool::saveToMatroska() {
  auto tab = currentTab();
  if (tab) {
    tab->saveToMatroska();
    enableMenuActions();
  }
}

void
Tool::reload() {
  auto tab = currentTab();
  if (!tab || tab->fileName().isEmpty())
    return;

  if (Util::Settings::get().m_warnBeforeClosingModifiedTabs && tab->hasBeenModified()) {
    auto answer = Util::MessageBox::question(this)
      ->title(QY("Reload modified file"))
      .text(QY("The file \"%1\" has been modified. Do you really want to reload it? All changes will be lost.").arg(tab->title()))
      .buttonLabel(QMessageBox::Yes, QY("&Reload file"))
      .buttonLabel(QMessageBox::No,  QY("Cancel"))
      .exec();
    if (answer != QMessageBox::Yes)
      return;
  }

  tab->load();
}

bool
Tool::closeTab(int index) {
  if ((0  > index) || (ui->editors->count() <= index))
    return false;

  auto tab = static_cast<Tab *>(ui->editors->widget(index));

  if (Util::Settings::get().m_warnBeforeClosingModifiedTabs && tab->hasBeenModified()) {
    MainWindow::get()->switchToTool(this);
    ui->editors->setCurrentIndex(index);
    auto answer = Util::MessageBox::question(this)
      ->title(QY("Close modified file"))
      .text(QY("The file \"%1\" has been modified. Do you really want to close? All changes will be lost.").arg(tab->title()))
      .buttonLabel(QMessageBox::Yes, QY("&Close file"))
      .buttonLabel(QMessageBox::No,  QY("Cancel"))
      .exec();
    if (answer != QMessageBox::Yes)
      return false;
  }

  ui->editors->removeTab(index);
  delete tab;

  showChapterEditorsWidget();

  return true;
}

void
Tool::closeCurrentTab() {
  closeTab(ui->editors->currentIndex());
}

void
Tool::closeSendingTab() {
  auto idx = ui->editors->indexOf(dynamic_cast<Tab *>(sender()));
  if (-1 != idx)
    closeTab(idx);
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
Tool::tabTitleChanged() {
  auto tab = dynamic_cast<Tab *>(sender());
  auto idx = ui->editors->indexOf(tab);
  if (tab && (-1 != idx))
    ui->editors->setTabText(idx, tab->title());
}

void
Tool::setupTabPositions() {
  ui->editors->setTabPosition(Util::Settings::get().m_tabPosition);
}

QString
Tool::chapterNameTemplateToolTip() {
  return Q("<p>%1 %2 %3</p><p>%4 %5</p><p>%6 %7 %8</p><ul><li>%9</li><li>%10</li><li>%11</li><li>%12</li><li>%13</li><li>%14</li><li>%15</li><li>%16</li></ul>")
    .arg(QY("This template will be used for new chapter entries."))
    .arg(QY("The string '<NUM>' will be replaced by the chapter number.").toHtmlEscaped())
    .arg(QY("The string '<START>' will be replaced by the chapter's start timestamp.").toHtmlEscaped())

    .arg(QY("You can specify a minimum number of places for the chapter number with '<NUM:places>', e.g. '<NUM:3>'.").toHtmlEscaped())
    .arg(QY("The resulting number will be padded with leading zeroes if the number of places is less than specified."))

    .arg(QY("You can control the format used by the start timestamp with <START:format>.").toHtmlEscaped())
    .arg(QY("The format defaults to %H:%M:%S if none is given."))
    .arg(QY("Valid format codes are:"))

    .arg(QY("%h – hours"))
    .arg(QY("%H – hours zero-padded to two places"))
    .arg(QY("%m – minutes"))
    .arg(QY("%M – minutes zero-padded to two places"))
    .arg(QY("%s – seconds"))
    .arg(QY("%S – seconds zero-padded to two places"))
    .arg(QY("%n – nanoseconds with nine places"))
    .arg(QY("%<1-9>n – nanoseconds with up to nine places (e.g. three places with %3n)").toHtmlEscaped());
}

}}}
