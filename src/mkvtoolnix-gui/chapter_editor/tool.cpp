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

  connect(mwUi->actionChapterEditorNew,    &QAction::triggered,         this, &Tool::newFile);
  connect(mwUi->actionChapterEditorOpen,   &QAction::triggered,         this, &Tool::selectFileToOpen);
  // connect(mwUi->actionChapterEditorSave,   &QAction::triggered,         this, &Tool::save);
  // connect(mwUi->actionChapterEditorReload, &QAction::triggered,         this, &Tool::reload);
  connect(mwUi->actionChapterEditorClose,  &QAction::triggered,         this, &Tool::closeCurrentTab);

  connect(ui->editors,                     &QTabWidget::currentChanged, this, &Tool::enableMenuActions);
}

void
Tool::showChapterEditorsWidget() {
  ui->stack->setCurrentWidget(ui->editors->count() ? ui->editorsPage : ui->noFilesPage);
  enableMenuActions();
}

void
Tool::enableMenuActions() {
  auto mwUi = MainWindow::get()->getUi();
  auto tab  = currentTab();

  mwUi->actionChapterEditorSave->setEnabled(tab && !tab->getFileName().isEmpty());
  mwUi->actionChapterEditorSaveAsXml->setEnabled(!!tab);
  mwUi->actionChapterEditorSaveToMatroska->setEnabled(!!tab);
  mwUi->actionChapterEditorReload->setEnabled(tab && !tab->getFileName().isEmpty());
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
                                                 QY("Supported file types")           + Q(" (*.mkv *.mka *.mks *.mk3d *.txt *.webm *.xml);;") +
                                                 QY("Matroska and WebM files")        + Q(" (*.mkv *.mka *.mks *.mk3d *.webm);;") +
                                                 QY("XML chapter files")              + Q(" (*.xml);;") +
                                                 QY("Simple OGM-style chapter files") + Q(" (*.txt);;") +
                                                 QY("All files")                      + Q(" (*)"));
  if (fileNames.isEmpty())
    return;

  MainWindow::get()->setStatusBarMessage(QNY("Opening %1 file in the chapter editor…", "Opening %1 files in the chapter editor…", fileNames.count()).arg(fileNames.count()));

  for (auto const &fileName : fileNames)
    openFile(fileName);
}

// void
// Tool::save() {
//   auto tab = currentTab();
//   if (tab)
//     tab->save();
// }

// void
// Tool::reload() {
//   auto tab = currentTab();
//   if (!tab)
//     return;

//   if (tab->hasBeenModified()) {
//     auto answer = QMessageBox::question(this, QY("File has been modified"), QY("The file »%1« has been modified. Do you really want to reload it? All changes will be lost.").arg(QFileInfo{tab->getFileName()}.fileName()),
//                                         QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
//     if (answer != QMessageBox::Yes)
//       return;
//   }

//   tab->load();
// }

void
Tool::closeTab(int index) {
  if ((0  > index) || (ui->editors->count() <= index))
    return;

  auto tab = static_cast<Tab *>(ui->editors->widget(index));

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
