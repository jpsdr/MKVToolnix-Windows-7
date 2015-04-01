#include "common/common_pch.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>

#include "common/qt.h"
#include "mkvtoolnix-gui/forms/header_editor_container_widget.h"
#include "mkvtoolnix-gui/forms/main_window.h"
#include "mkvtoolnix-gui/header_editor_widget/header_editor_container_widget.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/merge_widget/mux_config.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/util.h"

HeaderEditorContainerWidget::HeaderEditorContainerWidget(QWidget *parent,
                                                         QMenu *headerEditorMenu)
  : ToolBase{parent}
  , ui{new Ui::HeaderEditorContainerWidget}
  , m_headerEditorMenu{headerEditorMenu}
{
  // Setup UI controls.
  ui->setupUi(this);

  ui->headerEditors->setVisible(false);
  ui->noFileOpened->setVisible(true);

  setupMenu();

  showHeaderEditorsWidget();
}

HeaderEditorContainerWidget::~HeaderEditorContainerWidget() {
}

void
HeaderEditorContainerWidget::setupMenu() {
  auto mwUi = MainWindow::get()->getUi();

  connect(mwUi->actionHeaderEditorOpen,   SIGNAL(triggered()), this, SLOT(selectFileToOpen()));
  connect(mwUi->actionHeaderEditorSave,   SIGNAL(triggered()), this, SLOT(save()));
  connect(mwUi->actionHeaderEditorReload, SIGNAL(triggered()), this, SLOT(reload()));
  connect(mwUi->actionHeaderEditorClose,  SIGNAL(triggered()), this, SLOT(closeCurrentTab()));
}

void
HeaderEditorContainerWidget::showHeaderEditorsWidget() {
  auto hasTabs = !!ui->headerEditors->count();
  auto mwUi    = MainWindow::get()->getUi();

  ui->headerEditors->setVisible(hasTabs);
  ui->noFileOpened->setVisible(!hasTabs);

  mwUi->actionHeaderEditorSave->setEnabled(hasTabs);
  mwUi->actionHeaderEditorReload->setEnabled(hasTabs);
  mwUi->actionHeaderEditorClose->setEnabled(hasTabs);
}

void
HeaderEditorContainerWidget::toolShown() {
  MainWindow::get()->showTheseMenusOnly({ m_headerEditorMenu });
  showHeaderEditorsWidget();
}

void
HeaderEditorContainerWidget::retranslateUi() {
  ui->noFileOpenedLabel->setText(QY("No file has been opened yet."));
  ui->howToOpenLabel->setText(QY("Open a file via the \"header editor\" menu or drag & drop one here."));
}

void
HeaderEditorContainerWidget::dragEnterEvent(QDragEnterEvent *event) {
  if (!event->mimeData()->hasUrls())
    return;

  for (auto const &url : event->mimeData()->urls())
    if (!url.isLocalFile() || !QFileInfo{url.toLocalFile()}.isFile())
      return;

  event->acceptProposedAction();
}

void
HeaderEditorContainerWidget::dropEvent(QDropEvent *event) {
  if (!event->mimeData()->hasUrls())
    return;

  event->acceptProposedAction();

  for (auto const &url : event->mimeData()->urls())
    if (url.isLocalFile())
      openFile(url.toLocalFile());
}

void
HeaderEditorContainerWidget::openFile(QString const &fileName) {
  // TODO: HeaderEditorContainerWidget::openFile
  MainWindow::get()->setStatusBarMessage(fileName);

  Settings::get().m_lastMatroskaFileDir = QFileInfo{fileName}.path();
  Settings::get().save();

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

  // TODO: HeaderEditorContainerWidget::openFile
}

void
HeaderEditorContainerWidget::selectFileToOpen() {
  auto fileNames = QFileDialog::getOpenFileNames(this, QY("Open files in header editor"), Settings::get().m_lastMatroskaFileDir.path(),
                                                QY("Matroska and WebM files") + Q(" (*.mkv *.mka *.mks *.mk3d *.webm);;") + QY("All files") + Q(" (*)"));
  if (fileNames.isEmpty())
    return;

  for (auto const &fileName : fileNames)
    openFile(fileName);
}

void
HeaderEditorContainerWidget::save() {
  // TODO: HeaderEditorContainerWidget::save
}

void
HeaderEditorContainerWidget::reload() {
  // TODO: HeaderEditorContainerWidget::reload
}

void
HeaderEditorContainerWidget::closeTab(int index) {
  if ((0  > index) || (ui->headerEditors->count() <= index))
    return;

  auto widget = ui->headerEditors->widget(index);
  // TODO: HeaderEditorContainerWidget::closeTab: test if modified
  ui->headerEditors->removeTab(index);
  delete widget;

  showHeaderEditorsWidget();
}

void
HeaderEditorContainerWidget::closeCurrentTab() {
  closeTab(ui->headerEditors->currentIndex());
}
