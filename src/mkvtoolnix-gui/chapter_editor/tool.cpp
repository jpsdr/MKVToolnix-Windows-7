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
#include "mkvtoolnix-gui/util/string.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx::gui::ChapterEditor {

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

  MainWindow::get()->registerSubWindowWidget(*this, *ui->editors);
}

Tool::~Tool() {
}

void
Tool::setupUi() {
  Util::setupTabWidgetHeaders(*ui->editors);

  showChapterEditorsWidget();

  retranslateUi();

  connect(ui->editors, &QTabWidget::tabCloseRequested, this, &Tool::closeTab);
}

void
Tool::setupActions() {
  auto mw   = MainWindow::get();
  auto mwUi = MainWindow::getUi();

  connect(mwUi->actionChapterEditorNew,                &QAction::triggered,             this, &Tool::newFile);
  connect(mwUi->actionChapterEditorOpen,               &QAction::triggered,             this, [this]() { selectFileToOpen(false); });
  connect(mwUi->actionChapterEditorAppend,             &QAction::triggered,             this, [this]() { selectFileToOpen(true); });
  connect(mwUi->actionChapterEditorSave,               &QAction::triggered,             this, &Tool::save);
  connect(mwUi->actionChapterEditorSaveAsXml,          &QAction::triggered,             this, &Tool::saveAsXml);
  connect(mwUi->actionChapterEditorSaveToMatroska,     &QAction::triggered,             this, &Tool::saveToMatroska);
  connect(mwUi->actionChapterEditorSaveAll,            &QAction::triggered,             this, &Tool::saveAllTabs);
  connect(mwUi->actionChapterEditorReload,             &QAction::triggered,             this, &Tool::reload);
  connect(mwUi->actionChapterEditorClose,              &QAction::triggered,             this, &Tool::closeCurrentTab);
  connect(mwUi->actionChapterEditorCloseAll,           &QAction::triggered,             this, &Tool::closeAllTabs);
  connect(mwUi->actionChapterEditorRemoveFromMatroska, &QAction::triggered,             this, &Tool::removeChaptersFromExistingMatroskaFile);

  connect(ui->newFileButton,                           &QPushButton::clicked,           this, &Tool::newFile);
  connect(ui->openFileButton,                          &QPushButton::clicked,           this, [this]() { selectFileToOpen(false); });

  connect(m_chapterEditorMenu,                         &QMenu::aboutToShow,             this, &Tool::enableMenuActions);
  connect(mw,                                          &MainWindow::preferencesChanged, [this]() { Util::setupTabWidgetHeaders(*ui->editors); });
  connect(mw,                                          &MainWindow::preferencesChanged, this, &Tool::retranslateUi);

  connect(App::instance(),                             &App::editingChaptersRequested,  this, &Tool::openFilesFromCommandLine);
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
  auto isSourceKax = tab && tab->isSourceMatroska();

  mwUi->actionChapterEditorSave->setEnabled(          tabEnabled && (hasElements || isSourceKax) && hasFileName);
  mwUi->actionChapterEditorSaveAsXml->setEnabled(     tabEnabled && hasElements);
  mwUi->actionChapterEditorSaveToMatroska->setEnabled(tabEnabled);
  mwUi->actionChapterEditorReload->setEnabled(        tabEnabled                                 && hasFileName);
  mwUi->actionChapterEditorAppend->setEnabled(        !!tab);
  mwUi->actionChapterEditorClose->setEnabled(         !!tab);
  mwUi->menuChapterEditorAll->setEnabled(             !!tab);
  mwUi->actionChapterEditorSaveAll->setEnabled(       !!tab);
  mwUi->actionChapterEditorCloseAll->setEnabled(      !!tab);
}

void
Tool::toolShown() {
  MainWindow::get()->showTheseMenusOnly({ m_chapterEditorMenu });
  showChapterEditorsWidget();
}

void
Tool::retranslateUi() {
  auto buttonToolTip = Util::Settings::get().m_uiDisableToolTips ? Q("") : App::translate("CloseButton", "Close Tab");

  ui->retranslateUi(this);

  for (auto idx = 0, numTabs = ui->editors->count(); idx < numTabs; ++idx) {
    static_cast<Tab *>(ui->editors->widget(idx))->retranslateUi();
    auto button = Util::tabWidgetCloseTabButton(*ui->editors, idx);
    if (button)
      button->setToolTip(buttonToolTip);
  }
}

Tab *
Tool::appendTab(Tab *tab) {
  connect(tab, &Tab::removeThisTab,          this, &Tool::closeSendingTab);
  connect(tab, &Tab::titleChanged,           this, &Tool::tabTitleChanged);
  connect(tab, &Tab::numberOfEntriesChanged, this, &Tool::enableMenuActions);

  ui->editors->addTab(tab, Util::escape(tab->title(), Util::EscapeKeyboardShortcuts));

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
Tool::openFile(QString const &fileName,
               bool append) {
  auto &settings = Util::Settings::get();
  settings.m_lastOpenDir.setPath(QFileInfo{fileName}.path());
  settings.save();

  if (!append)
    appendTab(new Tab{this, fileName})->load();
  else if (currentTab())
    currentTab()->append(fileName);
}

void
Tool::openFiles(QStringList const &fileNames) {
  for (auto const &fileName : fileNames)
    openFile(fileName, false);
}

void
Tool::openFilesFromCommandLine(QStringList const &fileNames) {
  MainWindow::get()->switchToTool(this);
  openFiles(fileNames);
}

void
Tool::selectFileToOpen(bool append) {
  QString dvds, ifo;

#if defined(HAVE_DVDREAD)
  dvds = QY("DVDs") + Q(" (*.ifo *.IFO);;");
  ifo  = Q(" *.ifo *.IFO");
#endif  // HAVE_DVDREAD

  auto fileNames = Util::getOpenFileNames(this, append ? QY("Append files in chapter editor") : QY("Open files in chapter editor"), Util::Settings::get().lastOpenDirPath(),
                                          QY("Supported file types")           + Q(" (*.cue%1 *.meta *.mpls *.mkv *.mka *.mks *.mk3d *.pbf *.txt *.webm *.xml);;").arg(ifo) +
                                          QY("Matroska files")                 + Q(" (*.mkv *.mka *.mks *.mk3d);;") +
                                          QY("WebM files")                     + Q(" (*.webm);;") +
                                          QY("Blu-ray playlist files")         + Q(" (*.mpls);;") +
                                          dvds +
                                          QY("XML chapter files")              + Q(" (*.xml);;") +
                                          QY("Simple OGM-style chapter files") + Q(" (*.txt);;") +
                                          QY("Cue sheet files")                + Q(" (*.cue);;") +
                                          QY("ffmpeg metadata files")          + Q(" (*.meta);;") +
                                          QY("PotPlayer bookmark files")       + Q(" (*.pbf);;") +
                                          QY("All files")                      + Q(" (*)"));
  if (fileNames.isEmpty())
    return;

  MainWindow::get()->setStatusBarMessage(QNY("Opening %1 file in the chapter editor…", "Opening %1 files in the chapter editor…", fileNames.count()).arg(fileNames.count()));

  for (auto const &fileName : fileNames)
    openFile(fileName, append);
}

void
Tool::save() {
  auto tab = currentTab();
  if (tab)
    tab->save();
}

void
Tool::saveAllTabs() {
  forEachTab([](auto &tab) { tab.save(); });
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
    ui->editors->setTabText(idx, Util::escape(tab->title(), Util::EscapeKeyboardShortcuts));
}

QString
Tool::chapterNameTemplateToolTip() {
  return Q("<p>%1 %2 %3 %4</p><p>%5 %6</p><p>%7 %8 %9</p><ul><li>%10</li><li>%11</li><li>%12</li><li>%13</li><li>%14</li><li>%15</li><li>%16</li><li>%17</li><li>%18</li></ul>")
    .arg(QYH("This template will be used for new chapter entries."))
    .arg(QYH("The string '<NUM>' will be replaced by the chapter number."))
    .arg(QYH("The string '<START>' will be replaced by the chapter's start timestamp."))
    .arg(QYH("The strings '<FILE_NAME>' and '<FILE_NAME_WITH_EXT>', which only work when generating chapters for appended files, will be replaced by the appended file's name "
             "(the former leaves the extension out while the latter includes it)."))
    .arg(QYH("The string '<TITLE>', which only work when generating chapters for appended files, will be replaced by the appended file's title if one is set."))

    .arg(QYH("You can specify a minimum number of places for the chapter number with '<NUM:places>', e.g. '<NUM:3>'."))
    .arg(QYH("The resulting number will be padded with leading zeroes if the number of places is less than specified."))

    .arg(QYH("You can control the format used by the start timestamp with <START:format>."))
    .arg(QYH("The format defaults to %H:%M:%S if none is given."))
    .arg(QYH("Valid format codes are:"))

    .arg(QYH("%h – hours"))
    .arg(QYH("%H – hours zero-padded to two places"))
    .arg(QYH("%m – minutes"))
    .arg(QYH("%M – minutes zero-padded to two places"))
    .arg(QYH("%s – seconds"))
    .arg(QYH("%S – seconds zero-padded to two places"))
    .arg(QYH("%n – nanoseconds with nine places"))
    .arg(QYH("%<1-9>n – nanoseconds with up to nine places (e.g. three places with %{}3n)"))
    .remove(Q("{}"))
  + Q("<p>%1</p>")
    .arg(QYH("If nothing is entered, chapters will be generated but no name will be set."));
}

void
Tool::removeChaptersFromExistingMatroskaFile() {
  auto fileName = Util::getOpenFileName(this, QY("Removing chapters from existing Matroska file"), Util::Settings::get().lastOpenDirPath(),
                                        QY("Matroska files") + Q(" (*.mkv *.mka *.mks *.mk3d);;") +
                                        QY("All files")      + Q(" (*)"));
  if (fileName.isEmpty())
    return;

  auto analyzer = std::make_unique<Util::KaxAnalyzer>(this, fileName);

  if (!analyzer->set_parse_mode(kax_analyzer_c::parse_mode_fast).process()) {
    auto text = Q("%1 %2")
      .arg(QY("The file you tried to open (%1) could not be read successfully.").arg(fileName))
      .arg(QY("Possible reasons are: the file is not a Matroska file; the file is write-protected; the file is locked by another process; you do not have permission to access the file."));
    Util::MessageBox::critical(this)->title(QY("File parsing failed")).text(text).exec();
    return;
  }

  auto idx = analyzer->find(EBML_ID(libmatroska::KaxChapters));
  if (-1 == idx) {
    Util::MessageBox::information(this)->title(QY("Removing chapters from existing Matroska file")).text(QY("The file you tried to open (%1) does not contain any chapters.").arg(fileName)).exec();
    return;
  }

  auto result = analyzer->remove_elements(EBML_ID(libmatroska::KaxChapters));

  if (kax_analyzer_c::uer_success != result) {
    Util::KaxAnalyzer::displayUpdateElementResult(this, result, QY("Removing the chapters failed."));

  } else
    Util::MessageBox::information(this)->title(QY("Removing chapters from existing Matroska file")).text(QY("All chapters have been removed from the file '%1'.").arg(fileName)).exec();
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

std::pair<QString, QString>
Tool::nextPreviousWindowActionTexts()
  const {
  return {
    QY("&Next chapter editor tab"),
    QY("&Previous chapter editor tab"),
  };
}

QVector<Tab *>
Tool::tabs() {
  QVector<Tab *> t;

  for (auto idx = 0, numTabs = ui->editors->count(); idx < numTabs; ++idx)
    t << static_cast<Tab *>(ui->editors->widget(idx));

  return t;
}

}
