#include "common/common_pch.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>

#include "common/qt.h"
#include "mkvtoolnix-gui/forms/chapter_editor/tool.h"
#include "mkvtoolnix-gui/forms/main_window/main_window.h"
#include "mkvtoolnix-gui/chapter_editor/tab.h"
#include "mkvtoolnix-gui/chapter_editor/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/merge/mux_config.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/util.h"

namespace mtx { namespace gui { namespace ChapterEditor {

using namespace mtx::gui;

Tool::Tool(QWidget *parent,
           QMenu *chapterEditorMenu)
  : ToolBase{parent}
  , ui{new Ui::Tool}
  , m_chapterEditorMenu{chapterEditorMenu}
{
  // Setup UI controls.
  ui->setupUi(this);

  setupMenu();

  showChapterEditorsWidget();
}

Tool::~Tool() {
}

void
Tool::setupMenu() {
  auto mwUi = MainWindow::get()->getUi();

  connect(mwUi->actionChapterEditorNew,            &QAction::triggered,         this, &Tool::newFile);
  connect(mwUi->actionChapterEditorOpen,           &QAction::triggered,         this, &Tool::selectFileToOpen);
  connect(mwUi->actionChapterEditorSave,           &QAction::triggered,         this, &Tool::save);
  connect(mwUi->actionChapterEditorSaveAsXml,      &QAction::triggered,         this, &Tool::saveAsXml);
  connect(mwUi->actionChapterEditorSaveToMatroska, &QAction::triggered,         this, &Tool::saveToMatroska);
  connect(mwUi->actionChapterEditorReload,         &QAction::triggered,         this, &Tool::reload);
  connect(mwUi->actionChapterEditorClose,          &QAction::triggered,         this, &Tool::closeCurrentTab);

  connect(ui->editors,                             &QTabWidget::currentChanged, this, &Tool::enableMenuActions);
}

void
Tool::showChapterEditorsWidget() {
  ui->stack->setCurrentWidget(ui->editors->count() ? ui->editorsPage : ui->noFilesPage);
  enableMenuActions();
}

void
Tool::enableMenuActions() {
  auto mwUi        = MainWindow::get()->getUi();
  auto tab         = currentTab();
  auto hasFileName = tab && !tab->getFileName().isEmpty();
  auto hasElements = tab && !tab->isEmpty();

  mwUi->actionChapterEditorSave->setEnabled(hasFileName && hasElements);
  mwUi->actionChapterEditorSaveAsXml->setEnabled(!!tab && hasElements);
  mwUi->actionChapterEditorSaveToMatroska->setEnabled(!!tab && hasElements);
  mwUi->actionChapterEditorReload->setEnabled(hasFileName);
  mwUi->actionChapterEditorClose->setEnabled(!!tab);
}

void
Tool::toolShown() {
  MainWindow::get()->showTheseMenusOnly({ m_chapterEditorMenu });
  showChapterEditorsWidget();
}

void
Tool::retranslateUi() {
  ui->noFileOpenedLabel->setText(QY("No file has been opened yet."));
  ui->howToOpenLabel->setText(QY("Open a file via the \"chapter editor\" menu or drag & drop one here."));
}

Tab *
Tool::appendTab(Tab *tab) {
  connect(tab, &Tab::removeThisTab, this, &Tool::closeSendingTab);
  connect(tab, &Tab::titleChanged,  this, &Tool::tabTitleChanged);

  ui->editors->addTab(tab, tab->getTitle());

  showChapterEditorsWidget();

  ui->editors->setCurrentIndex(ui->editors->count() - 1);

  return tab;
}

void
Tool::newFile() {
   appendTab(new Tab{this})
     ->newFile();
}

// TODO: Tool::dragEnterEvent: drag & drop
// void
// Tool::dragEnterEvent(QDragEnterEvent *event) {
//   if (!event->mimeData()->hasUrls())
//     return;

//   for (auto const &url : event->mimeData()->urls())
//     if (!url.isLocalFile() || !QFileInfo{url.toLocalFile()}.isFile())
//       return;

//   event->acceptProposedAction();
// }

// void
// Tool::dropEvent(QDropEvent *event) {
//   if (!event->mimeData()->hasUrls())
//     return;

//   event->acceptProposedAction();

//   for (auto const &url : event->mimeData()->urls())
//     if (url.isLocalFile())
//       openFile(url.toLocalFile());
// }

void
Tool::openFile(QString const &fileName) {
  auto &settings = Util::Settings::get();
  settings.m_lastMatroskaFileDir = QFileInfo{fileName}.path();
  settings.save();

  appendTab(new Tab{this, fileName})
   ->load();
}

void
Tool::selectFileToOpen() {
  auto fileNames = QFileDialog::getOpenFileNames(this, QY("Open files in chapter editor"), Util::Settings::get().m_lastMatroskaFileDir.path(),
                                                 QY("Supported file types")           + Q(" (*.mkv *.mka *.mks *.mk3d *.txt *.xml);;") +
                                                 QY("Matroska files")                 + Q(" (*.mkv *.mka *.mks *.mk3d);;") +
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
  if (tab)
    tab->saveAsXml();
}

void
Tool::saveToMatroska() {
  auto tab = currentTab();
  if (tab)
    tab->saveToMatroska();
}

void
Tool::reload() {
  auto tab = currentTab();
  if (!tab || tab->getFileName().isEmpty())
    return;

  // TODO: Tool::reload: hasBeenModified
//   if (tab->hasBeenModified()) {
//     auto answer = QMessageBox::question(this, QY("File has been modified"), QY("The file »%1« has been modified. Do you really want to reload it? All changes will be lost.").arg(QFileInfo{tab->getFileName()}.fileName()),
//                                         QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
//     if (answer != QMessageBox::Yes)
//       return;
//   }

  tab->load();
}

void
Tool::closeTab(int index) {
  if ((0  > index) || (ui->editors->count() <= index))
    return;

  auto tab = static_cast<Tab *>(ui->editors->widget(index));

  // TODO: Tool::closeTab: hasBeenModified
  // if (tab->hasBeenModified()) {
  //   auto answer = QMessageBox::question(this, QY("File has been modified"), QY("The file »%1« has been modified. Do you really want to close? All changes will be lost.").arg(QFileInfo{tab->getFileName()}.fileName()),
  //                                       QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
  //   if (answer != QMessageBox::Yes)
  //     return;
  // }

  ui->editors->removeTab(index);
  delete tab;

  showChapterEditorsWidget();
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

Tab *
Tool::currentTab() {
  return static_cast<Tab *>(ui->editors->widget(ui->editors->currentIndex()));
}

void
Tool::tabTitleChanged(QString const &title) {
  auto idx = ui->editors->indexOf(dynamic_cast<Tab *>(sender()));
  if (-1 != idx)
    ui->editors->setTabText(idx, title);
}

}}}
