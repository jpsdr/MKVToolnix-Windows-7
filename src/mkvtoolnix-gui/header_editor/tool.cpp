#include "common/common_pch.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>

#include "common/qt.h"
#include "mkvtoolnix-gui/forms/header_editor/tool.h"
#include "mkvtoolnix-gui/forms/main_window.h"
#include "mkvtoolnix-gui/header_editor/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/merge/mux_config.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/util.h"

namespace mtx { namespace gui { namespace HeaderEditor {

Tool::Tool(QWidget *parent,
           QMenu *headerEditorMenu)
  : ToolBase{parent}
  , ui{new Ui::Tool}
  , m_headerEditorMenu{headerEditorMenu}
{
  // Setup UI controls.
  ui->setupUi(this);

  ui->headerEditors->setVisible(false);
  ui->noFileOpened->setVisible(true);

  setupMenu();

  showHeaderEditorsWidget();
}

Tool::~Tool() {
}

void
Tool::setupMenu() {
  auto mwUi = MainWindow::get()->getUi();

  connect(mwUi->actionHeaderEditorOpen,   SIGNAL(triggered()), this, SLOT(selectFileToOpen()));
  connect(mwUi->actionHeaderEditorSave,   SIGNAL(triggered()), this, SLOT(save()));
  connect(mwUi->actionHeaderEditorReload, SIGNAL(triggered()), this, SLOT(reload()));
  connect(mwUi->actionHeaderEditorClose,  SIGNAL(triggered()), this, SLOT(closeCurrentTab()));
}

void
Tool::showHeaderEditorsWidget() {
  auto hasTabs = !!ui->headerEditors->count();
  auto mwUi    = MainWindow::get()->getUi();

  ui->headerEditors->setVisible(hasTabs);
  ui->noFileOpened->setVisible(!hasTabs);

  mwUi->actionHeaderEditorSave->setEnabled(hasTabs);
  mwUi->actionHeaderEditorReload->setEnabled(hasTabs);
  mwUi->actionHeaderEditorClose->setEnabled(hasTabs);
}

void
Tool::toolShown() {
  MainWindow::get()->showTheseMenusOnly({ m_headerEditorMenu });
  showHeaderEditorsWidget();
}

void
Tool::retranslateUi() {
  ui->noFileOpenedLabel->setText(QY("No file has been opened yet."));
  ui->howToOpenLabel->setText(QY("Open a file via the \"header editor\" menu or drag & drop one here."));
}

void
Tool::dragEnterEvent(QDragEnterEvent *event) {
  if (!event->mimeData()->hasUrls())
    return;

  for (auto const &url : event->mimeData()->urls())
    if (!url.isLocalFile() || !QFileInfo{url.toLocalFile()}.isFile())
      return;

  event->acceptProposedAction();
}

void
Tool::dropEvent(QDropEvent *event) {
  if (!event->mimeData()->hasUrls())
    return;

  event->acceptProposedAction();

  for (auto const &url : event->mimeData()->urls())
    if (url.isLocalFile())
      openFile(url.toLocalFile());
}

void
Tool::openFile(QString const &fileName) {
  // TODO: Tool::openFile
  MainWindow::get()->setStatusBarMessage(fileName);

  auto &settings = mtx::gui::Util::Settings::get();
  settings.m_lastMatroskaFileDir = QFileInfo{fileName}.path();
  settings.save();

  if (!kax_analyzer_c::probe(to_utf8(fileName))) {
    QMessageBox::critical(this, QY("File parsing failed"), QY("The file you tried to open (%1) is not recognized as a valid Matroska/WebM file.").arg(fileName));
    return;
  }

  // m_e_segment_info.reset();
  // m_e_tracks.reset();

  auto analyzer = std::make_unique<QtKaxAnalyzer>(this, fileName);

  if (!analyzer->process(kax_analyzer_c::parse_mode_full)) {
    QMessageBox::critical(this, QY("File parsing failed"), QY("The file you tried to open (%1) could not be read successfully.").arg(fileName));
    return;
  }

  // ui->headerEditors->addTab(new HeaderEditorTab{fileName, std::move(analyzer)}, QFileInfo{fileName}.fileName());
  ui->headerEditors->addTab(new QWidget{}, QFileInfo{fileName}.fileName());

  showHeaderEditorsWidget();

  ui->headerEditors->setCurrentIndex(ui->headerEditors->count() - 1);

  // TODO: Tool::openFile
}

void
Tool::selectFileToOpen() {
  auto fileNames = QFileDialog::getOpenFileNames(this, QY("Open files in header editor"), mtx::gui::Util::Settings::get().m_lastMatroskaFileDir.path(),
                                                QY("Matroska and WebM files") + Q(" (*.mkv *.mka *.mks *.mk3d *.webm);;") + QY("All files") + Q(" (*)"));
  if (fileNames.isEmpty())
    return;

  for (auto const &fileName : fileNames)
    openFile(fileName);
}

void
Tool::save() {
  // TODO: Tool::save
}

void
Tool::reload() {
  // TODO: Tool::reload
}

void
Tool::closeTab(int index) {
  if ((0  > index) || (ui->headerEditors->count() <= index))
    return;

  auto widget = ui->headerEditors->widget(index);
  // TODO: Tool::closeTab: test if modified
  ui->headerEditors->removeTab(index);
  delete widget;

  showHeaderEditorsWidget();
}

void
Tool::closeCurrentTab() {
  closeTab(ui->headerEditors->currentIndex());
}

}}}
