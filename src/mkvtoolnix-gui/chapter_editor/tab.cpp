#include "common/common_pch.h"

#include <QDir>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>
#include <QTimer>

#include <matroska/KaxSemantic.h>

#include "common/bitvalue.h"
#include "common/chapters/chapters.h"
#include "common/construct.h"
#include "common/ebml.h"
#include "common/mm_io_x.h"
#include "common/mpls.h"
#include "common/qt.h"
#include "common/segmentinfo.h"
#include "common/segment_tracks.h"
#include "common/strings/formatting.h"
#include "common/strings/parsing.h"
#include "common/translation.h"
#include "common/xml/ebml_chapters_converter.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/forms/chapter_editor/tab.h"
#include "mkvtoolnix-gui/chapter_editor/generate_sub_chapters_parameters_dialog.h"
#include "mkvtoolnix-gui/chapter_editor/name_model.h"
#include "mkvtoolnix-gui/chapter_editor/mass_modification_dialog.h"
#include "mkvtoolnix-gui/chapter_editor/tab.h"
#include "mkvtoolnix-gui/chapter_editor/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/main_window/select_character_set_dialog.h"
#include "mkvtoolnix-gui/util/file.h"
#include "mkvtoolnix-gui/util/file_dialog.h"
#include "mkvtoolnix-gui/util/header_view_manager.h"
#include "mkvtoolnix-gui/util/message_box.h"
#include "mkvtoolnix-gui/util/model.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx { namespace gui { namespace ChapterEditor {

using namespace mtx::gui;

Tab::Tab(QWidget *parent,
         QString const &fileName)
  : QWidget{parent}
  , ui{new Ui::Tab}
  , m_fileName{fileName}
  , m_chapterModel{new ChapterModel{this}}
  , m_nameModel{new NameModel{this}}
  , m_expandAllAction{new QAction{this}}
  , m_collapseAllAction{new QAction{this}}
  , m_addEditionBeforeAction{new QAction{this}}
  , m_addEditionAfterAction{new QAction{this}}
  , m_addChapterBeforeAction{new QAction{this}}
  , m_addChapterAfterAction{new QAction{this}}
  , m_addSubChapterAction{new QAction{this}}
  , m_removeElementAction{new QAction{this}}
  , m_duplicateAction{new QAction{this}}
  , m_massModificationAction{new QAction{this}}
  , m_generateSubChaptersAction{new QAction{this}}
  , m_renumberSubChaptersAction{new QAction{this}}
{
  // Setup UI controls.
  ui->setupUi(this);

  setupUi();

  retranslateUi();
}

Tab::~Tab() {
}

void
Tab::setupUi() {
  Util::Settings::get().handleSplitterSizes(ui->chapterEditorSplitter);

  ui->elements->setModel(m_chapterModel);
  ui->tvChNames->setModel(m_nameModel);

  ui->cbChNameCountry ->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

  ui->cbChNameLanguage->setup();
  ui->cbChNameCountry->setup(true);

  m_nameWidgets << ui->pbChRemoveName
                << ui->lChName         << ui->leChName
                << ui->lChNameLanguage << ui->cbChNameLanguage
                << ui->lChNameCountry  << ui->cbChNameCountry;

  Util::fixScrollAreaBackground(ui->scrollArea);
  Util::fixComboBoxViewWidth(*ui->cbChNameLanguage);
  Util::fixComboBoxViewWidth(*ui->cbChNameCountry);
  Util::HeaderViewManager::create(*ui->elements,  "ChapterEditor::Elements");
  Util::HeaderViewManager::create(*ui->tvChNames, "ChapterEditor::ChapterNames");

  m_addEditionBeforeAction->setIcon(QIcon{Q(":/icons/16x16/edit-table-insert-row-above.png")});
  m_addEditionAfterAction->setIcon(QIcon{Q(":/icons/16x16/edit-table-insert-row-below.png")});
  m_addChapterBeforeAction->setIcon(QIcon{Q(":/icons/16x16/edit-table-insert-row-above.png")});
  m_addChapterAfterAction->setIcon(QIcon{Q(":/icons/16x16/edit-table-insert-row-below.png")});
  m_addSubChapterAction->setIcon(QIcon{Q(":/icons/16x16/edit-table-insert-row-under.png")});
  m_generateSubChaptersAction->setIcon(QIcon{Q(":/icons/16x16/.png")});
  m_duplicateAction->setIcon(QIcon{Q(":/icons/16x16/tab-duplicate.png")});
  m_removeElementAction->setIcon(QIcon{Q(":/icons/16x16/list-remove.png")});
  m_renumberSubChaptersAction->setIcon(QIcon{Q(":/icons/16x16/format-list-ordered.png")});
  m_massModificationAction->setIcon(QIcon{Q(":/icons/16x16/tools-wizard.png")});

  auto mw = MainWindow::get();
  connect(ui->elements,                    &Util::BasicTreeView::customContextMenuRequested,                       this,                 &Tab::showChapterContextMenu);
  connect(ui->elements,                    &Util::BasicTreeView::deletePressed,                                    this,                 &Tab::removeElement);
  connect(ui->elements,                    &Util::BasicTreeView::insertPressed,                                    this,                 &Tab::addEditionOrChapterAfter);
  connect(ui->elements->selectionModel(),  &QItemSelectionModel::selectionChanged,                                 this,                 &Tab::chapterSelectionChanged);
  connect(ui->tvChNames->selectionModel(), &QItemSelectionModel::selectionChanged,                                 this,                 &Tab::nameSelectionChanged);
  connect(ui->leChName,                    &QLineEdit::textEdited,                                                 this,                 &Tab::chapterNameEdited);
  connect(ui->cbChNameLanguage,            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,                 &Tab::chapterNameLanguageChanged);
  connect(ui->cbChNameCountry,             static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,                 &Tab::chapterNameCountryChanged);
  connect(ui->pbChAddName,                 &QPushButton::clicked,                                                  this,                 &Tab::addChapterName);
  connect(ui->pbChRemoveName,              &QPushButton::clicked,                                                  this,                 &Tab::removeChapterName);
  connect(ui->pbBrowseSegmentUID,          &QPushButton::clicked,                                                  this,                 &Tab::addSegmentUIDFromFile);

  connect(m_expandAllAction,               &QAction::triggered,                                                    this,                 &Tab::expandAll);
  connect(m_collapseAllAction,             &QAction::triggered,                                                    this,                 &Tab::collapseAll);
  connect(m_addEditionBeforeAction,        &QAction::triggered,                                                    this,                 &Tab::addEditionBefore);
  connect(m_addEditionAfterAction,         &QAction::triggered,                                                    this,                 &Tab::addEditionAfter);
  connect(m_addChapterBeforeAction,        &QAction::triggered,                                                    this,                 &Tab::addChapterBefore);
  connect(m_addChapterAfterAction,         &QAction::triggered,                                                    this,                 &Tab::addChapterAfter);
  connect(m_addSubChapterAction,           &QAction::triggered,                                                    this,                 &Tab::addSubChapter);
  connect(m_removeElementAction,           &QAction::triggered,                                                    this,                 &Tab::removeElement);
  connect(m_duplicateAction,               &QAction::triggered,                                                    this,                 &Tab::duplicateElement);
  connect(m_massModificationAction,        &QAction::triggered,                                                    this,                 &Tab::massModify);
  connect(m_generateSubChaptersAction,     &QAction::triggered,                                                    this,                 &Tab::generateSubChapters);
  connect(m_renumberSubChaptersAction,     &QAction::triggered,                                                    this,                 &Tab::renumberSubChapters);

  connect(mw,                              &MainWindow::preferencesChanged,                                        ui->cbChNameLanguage, &Util::ComboBoxBase::reInitialize);
  connect(mw,                              &MainWindow::preferencesChanged,                                        ui->cbChNameCountry,  &Util::ComboBoxBase::reInitialize);

  for (auto &lineEdit : findChildren<Util::BasicLineEdit *>()) {
    lineEdit->acceptDroppedFiles(false).setTextToDroppedFileName(false);
    connect(lineEdit, &Util::BasicLineEdit::returnPressed,      this, &Tab::focusOtherControlInNextChapterElement);
    connect(lineEdit, &Util::BasicLineEdit::shiftReturnPressed, this, &Tab::focusSameControlInNextChapterElement);
  }
}

void
Tab::updateFileNameDisplay() {
  if (!m_fileName.isEmpty()) {
    auto info = QFileInfo{m_fileName};
    ui->fileName->setText(info.fileName());
    ui->directory->setText(QDir::toNativeSeparators(info.path()));

  } else {
    ui->fileName->setText(QY("<unsaved file>"));
    ui->directory->setText(Q(""));

  }
}

void
Tab::retranslateUi() {
  ui->retranslateUi(this);

  updateFileNameDisplay();

  m_expandAllAction->setText(QY("&Expand all"));
  m_collapseAllAction->setText(QY("&Collapse all"));
  m_addEditionBeforeAction->setText(QY("Add new e&dition before"));
  m_addEditionAfterAction->setText(QY("Add new ed&ition after"));
  m_addChapterBeforeAction->setText(QY("Add new c&hapter before"));
  m_addChapterAfterAction->setText(QY("Add new ch&apter after"));
  m_addSubChapterAction->setText(QY("Add new &sub-chapter inside"));
  m_removeElementAction->setText(QY("&Remove selected edition or chapter"));
  m_duplicateAction->setText(QY("D&uplicate selected edition or chapter"));
  m_massModificationAction->setText(QY("Additional &modifications"));
  m_generateSubChaptersAction->setText(QY("&Generate sub-chapters"));
  m_renumberSubChaptersAction->setText(QY("Re&number sub-chapters"));

  Util::setToolTip(ui->pbBrowseSegmentUID, QY("Select an existing Matroska or WebM file and the GUI will add its segment UID to the input field on the left."));

  m_chapterModel->retranslateUi();
  m_nameModel->retranslateUi();

  resizeChapterColumnsToContents();

  emit titleChanged();
}

void
Tab::resizeChapterColumnsToContents()
  const {
  Util::resizeViewColumnsToContents(ui->elements);
}

void
Tab::resizeNameColumnsToContents()
  const {
  Util::resizeViewColumnsToContents(ui->tvChNames);
}

QString
Tab::title()
  const {
  if (m_fileName.isEmpty())
    return QY("<unsaved file>");
  return QFileInfo{m_fileName}.fileName();
}

QString const &
Tab::fileName()
  const {
  return m_fileName;
}

void
Tab::newFile() {
  addEdition(false);

  auto selectionModel = ui->elements->selectionModel();
  auto selection      = QItemSelection{m_chapterModel->index(0, 0), m_chapterModel->index(0, m_chapterModel->columnCount() - 1)};
  selectionModel->select(selection, QItemSelectionModel::ClearAndSelect);

  addSubChapter();

  auto parentIdx = m_chapterModel->index(0, 0);
  selection      = QItemSelection{m_chapterModel->index(0, 0, parentIdx), m_chapterModel->index(0, m_chapterModel->columnCount() - 1, parentIdx)};
  selectionModel->select(selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current);

  resizeChapterColumnsToContents();

  ui->leChStart->selectAll();
  ui->leChStart->setFocus();

  m_savedState = currentState();
}

void
Tab::resetData() {
  m_analyzer.reset();
  m_nameModel->reset();
  m_chapterModel->reset();
}

Tab::LoadResult
Tab::loadFromMatroskaFile() {
  m_analyzer = std::make_unique<QtKaxAnalyzer>(this, m_fileName);

  if (!m_analyzer->set_parse_mode(kax_analyzer_c::parse_mode_fast).set_open_mode(MODE_READ).process()) {
    auto text = Q("%1 %2")
      .arg(QY("The file you tried to open (%1) could not be read successfully.").arg(m_fileName))
      .arg(QY("Possible reasons are: the file is not a Matroska file; the file is write-protected; the file is locked by another process; you do not have permission to access the file."));
    Util::MessageBox::critical(this)->title(QY("File parsing failed")).text(text).exec();
    emit removeThisTab();
    return {};
  }

  auto idx = m_analyzer->find(KaxChapters::ClassInfos.GlobalId);
  if (-1 == idx) {
    Util::MessageBox::critical(this)->title(QY("File parsing failed")).text(QY("The file you tried to open (%1) does not contain any chapters.").arg(m_fileName)).exec();
    emit removeThisTab();
    return {};
  }

  auto chapters = m_analyzer->read_element(idx);
  if (!chapters) {
    Util::MessageBox::critical(this)->title(QY("File parsing failed")).text(QY("The file you tried to open (%1) could not be read successfully.").arg(m_fileName)).exec();
    emit removeThisTab();
  }

  m_analyzer->close_file();

  return { std::static_pointer_cast<KaxChapters>(chapters), true };
}

Tab::LoadResult
Tab::checkSimpleFormatForBomAndNonAscii(ChaptersPtr const &chapters) {
  auto result = Util::checkForBomAndNonAscii(m_fileName);
  if ((BO_NONE != result.byteOrder) || !result.containsNonAscii)
    return { chapters, false };

  Util::enableChildren(this, false);

  m_originalFileName = m_fileName;
  auto dlg           = new SelectCharacterSetDialog{this, m_originalFileName};

  connect(dlg, &SelectCharacterSetDialog::characterSetSelected, this, &Tab::reloadSimpleChaptersWithCharacterSet);
  connect(dlg, &SelectCharacterSetDialog::rejected,             this, &Tab::closeTab);

  dlg->show();

  return {};
}

Tab::LoadResult
Tab::loadFromChapterFile() {
  auto isSimpleFormat = false;
  auto chapters       = ChaptersPtr{};
  auto error          = QString{};

  try {
    chapters = parse_chapters(to_utf8(m_fileName), 0, -1, 0, "", "", true, &isSimpleFormat);

  } catch (mtx::mm_io::exception &ex) {
    error = Q(ex.what());

  } catch (mtx::chapter_parser_x &ex) {
    error = Q(ex.what());
  }

  if (!chapters) {
    auto message = QY("The file you tried to open (%1) is recognized as neither a valid Matroska nor a valid chapter file.").arg(m_fileName);
    if (!error.isEmpty())
      message = Q("%1 %2").arg(message).arg(QY("Error message from the parser: %1").arg(error));

    Util::MessageBox::critical(this)->title(QY("File parsing failed")).text(message).exec();
    emit removeThisTab();

  } else if (isSimpleFormat) {
    auto result = checkSimpleFormatForBomAndNonAscii(chapters);

    m_fileName.clear();
    emit titleChanged();

    return result;
  }

  return { chapters, !isSimpleFormat };
}

void
Tab::reloadSimpleChaptersWithCharacterSet(QString const &characterSet) {
  auto error = QString{};

  try {
    auto chapters = parse_chapters(to_utf8(m_originalFileName), 0, -1, 0, "", to_utf8(characterSet), true);
    chaptersLoaded(chapters, false);

    Util::enableChildren(this, true);

    MainWindow::chapterEditorTool()->enableMenuActions();

    return;

  } catch (mtx::mm_io::exception &ex) {
    error = Q(ex.what());
  } catch (mtx::chapter_parser_x &ex) {
    error = Q(ex.what());
  }

  Util::MessageBox::critical(this)->title(QY("File parsing failed")).text(QY("Error message from the parser: %1").arg(error)).exec();

  emit removeThisTab();
}

bool
Tab::areWidgetsEnabled()
  const {
  return ui->elements->isEnabled();
}

ChaptersPtr
Tab::timecodesToChapters(std::vector<timestamp_c> const &timecodes)
  const {
  auto &cfg     = Util::Settings::get();
  auto chapters = ChaptersPtr{ static_cast<KaxChapters *>(mtx::construct::cons<KaxChapters>(mtx::construct::cons<KaxEditionEntry>())) };
  auto &edition = GetChild<KaxEditionEntry>(*chapters);
  auto idx      = 0;

  for (auto const &timecode : timecodes) {
    auto nameTemplate = QString{ cfg.m_chapterNameTemplate };
    auto name         = formatChapterName(nameTemplate, ++idx, timecode);
    auto atom         = mtx::construct::cons<KaxChapterAtom>(new KaxChapterTimeStart, timecode.to_ns(),
                                                             mtx::construct::cons<KaxChapterDisplay>(new KaxChapterString,   name,
                                                                                                     new KaxChapterLanguage, to_utf8(cfg.m_defaultChapterLanguage)));
    if (!cfg.m_defaultChapterCountry.isEmpty())
      GetChild<KaxChapterCountry>(GetChild<KaxChapterDisplay>(atom)).SetValue(to_utf8(cfg.m_defaultChapterCountry));

    edition.PushElement(*atom);
  }

  return chapters;
}

Tab::LoadResult
Tab::loadFromMplsFile() {
  auto chapters = ChaptersPtr{};
  auto error    = QString{};

  try {
    auto in     = mm_file_io_c{to_utf8(m_fileName)};
    auto parser = mpls::parser_c{};

    parser.enable_dropping_last_entry_if_at_end(Util::Settings::get().m_dropLastChapterFromBlurayPlaylist);

    if (parser.parse(&in))
      chapters = timecodesToChapters(parser.get_chapters());

  } catch (mtx::mm_io::exception &ex) {
    error = Q(ex.what());

  } catch (mtx::mpls::exception &ex) {
    error = Q(ex.what());

  }

  if (!chapters) {
    auto message = QY("The file you tried to open (%1) is recognized as neither a valid Matroska nor a valid chapter file.").arg(m_fileName);
    if (!error.isEmpty())
      message = Q("%1 %2").arg(message).arg(QY("Error message from the parser: %1").arg(error));

    Util::MessageBox::critical(this)->title(QY("File parsing failed")).text(message).exec();
    emit removeThisTab();

  } else {
    m_fileName.clear();
    emit titleChanged();
  }

  return { chapters, false };
}

void
Tab::load() {
  resetData();

  m_savedState = currentState();
  auto result  = kax_analyzer_c::probe(to_utf8(m_fileName)) ? loadFromMatroskaFile()
               : m_fileName.toLower().endsWith(Q(".mpls"))  ? loadFromMplsFile()
               :                                              loadFromChapterFile();

  if (result.first)
    chaptersLoaded(result.first, result.second);
}

void
Tab::chaptersLoaded(ChaptersPtr const &chapters,
                    bool canBeWritten) {
  fix_chapter_country_codes(*chapters);

  if (!m_fileName.isEmpty())
    m_fileModificationTime = QFileInfo{m_fileName}.lastModified();

  disconnect(m_chapterModel, &QStandardItemModel::rowsInserted, this, &Tab::expandInsertedElements);

  m_chapterModel->reset();
  m_chapterModel->populate(*chapters);

  if (canBeWritten)
    m_savedState = currentState();

  expandAll();
  resizeChapterColumnsToContents();

  connect(m_chapterModel, &QStandardItemModel::rowsInserted, this, &Tab::expandInsertedElements);

  MainWindow::chapterEditorTool()->enableMenuActions();
}

void
Tab::save() {
  if (!m_analyzer)
    saveAsXmlImpl(false);

  else
    saveToMatroskaImpl(false);
}

void
Tab::saveAsImpl(bool requireNewFileName,
                std::function<bool(bool, QString &)> const &worker) {
  if (!copyControlsToStorage())
    return;

  m_chapterModel->fixMandatoryElements();
  setControlsFromStorage();

  auto newFileName = m_fileName;
  if (m_fileName.isEmpty())
    requireNewFileName = true;

  if (!worker(requireNewFileName, newFileName))
    return;

  m_savedState = currentState();

  if (newFileName != m_fileName) {
    m_fileName             = newFileName;

    auto &settings         = Util::Settings::get();
    settings.m_lastOpenDir = QFileInfo{newFileName}.path();
    settings.save();

    updateFileNameDisplay();
    emit titleChanged();
  }

  MainWindow::get()->setStatusBarMessage(QY("The file has been saved successfully."));
}

void
Tab::saveAsXml() {
  saveAsXmlImpl(true);
}

void
Tab::saveAsXmlImpl(bool requireNewFileName) {
  saveAsImpl(requireNewFileName, [this](bool doRequireNewFileName, QString &newFileName) -> bool {
    if (doRequireNewFileName) {
      auto defaultFilePath = !m_fileName.isEmpty() ? Util::dirPath(QFileInfo{m_fileName}.path()) : Util::Settings::get().lastOpenDirPath();
      newFileName          = Util::getSaveFileName(this, QY("Save chapters as XML"), defaultFilePath, QY("XML chapter files") + Q(" (*.xml);;") + QY("All files") + Q(" (*)"));

      if (newFileName.isEmpty())
        return false;
    }

    try {
      auto chapters = m_chapterModel->allChapters();
      auto out      = mm_file_io_c{to_utf8(newFileName), MODE_CREATE};
      mtx::xml::ebml_chapters_converter_c::write_xml(*chapters, out);

    } catch (mtx::mm_io::exception &) {
      Util::MessageBox::critical(this)->title(QY("Saving failed")).text(QY("Creating the file failed. Check to make sure you have permission to write to that directory and that the drive is not full.")).exec();
      return false;

    } catch (mtx::xml::conversion_x &ex) {
      Util::MessageBox::critical(this)->title(QY("Saving failed")).text(QY("Converting the chapters to XML failed: %1").arg(ex.what())).exec();
      return false;
    }

    return true;
  });
}

void
Tab::saveToMatroska() {
  saveToMatroskaImpl(true);
}

void
Tab::saveToMatroskaImpl(bool requireNewFileName) {
  saveAsImpl(requireNewFileName, [this](bool doRequireNewFileName, QString &newFileName) -> bool {
    if (!m_analyzer)
      doRequireNewFileName = true;

    if (doRequireNewFileName) {
      auto defaultFilePath = !m_fileName.isEmpty() ? QFileInfo{m_fileName}.path() : Util::Settings::get().lastOpenDirPath();
      newFileName          = Util::getOpenFileName(this, QY("Save chapters to Matroska file"), defaultFilePath, QY("Matroska files") + Q(" (*.mkv *.mka *.mks *.mk3d);;") + QY("All files") + Q(" (*)"));

      if (newFileName.isEmpty())
        return false;
    }

    if (doRequireNewFileName || (QFileInfo{newFileName}.lastModified() != m_fileModificationTime)) {
      m_analyzer = std::make_unique<QtKaxAnalyzer>(this, newFileName);
      if (!m_analyzer->set_parse_mode(kax_analyzer_c::parse_mode_fast).process()) {
        auto text = Q("%1 %2")
          .arg(QY("The file you tried to open (%1) could not be read successfully.").arg(newFileName))
          .arg(QY("Possible reasons are: the file is not a Matroska file; the file is write-protected; the file is locked by another process; you do not have permission to access the file."));
        Util::MessageBox::critical(this)->title(QY("File parsing failed")).text(text).exec();
        return false;
      }

      m_fileName = newFileName;
    }

    auto chapters = m_chapterModel->allChapters();
    auto result   = kax_analyzer_c::uer_success;

    if (chapters && (0 != chapters->ListSize()))
      result = m_analyzer->update_element(chapters, true);
    else
      result = m_analyzer->remove_elements(EBML_ID(KaxChapters));

    m_analyzer->close_file();

    if (kax_analyzer_c::uer_success != result) {
      QtKaxAnalyzer::displayUpdateElementResult(this, result, QY("Saving the chapters failed."));
      return false;
    }

    m_fileModificationTime = QFileInfo{m_fileName}.lastModified();

    return true;
  });
}

void
Tab::selectChapterRow(QModelIndex const &idx,
                      bool ignoreSelectionChanges) {
  auto selection = QItemSelection{idx.sibling(idx.row(), 0), idx.sibling(idx.row(), m_chapterModel->columnCount() - 1)};

  m_ignoreChapterSelectionChanges = ignoreSelectionChanges;
  ui->elements->selectionModel()->setCurrentIndex(idx.sibling(idx.row(), 0), QItemSelectionModel::ClearAndSelect);
  ui->elements->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
  m_ignoreChapterSelectionChanges = false;
}

bool
Tab::copyControlsToStorage() {
  auto idx = Util::selectedRowIdx(ui->elements);
  return idx.isValid() ? copyControlsToStorage(idx) : true;
}

bool
Tab::copyControlsToStorage(QModelIndex const &idx) {
  auto result = copyControlsToStorageImpl(idx);
  if (result.first) {
    m_chapterModel->updateRow(idx);
    return true;
  }

  selectChapterRow(idx, true);

  Util::MessageBox::critical(this)->title(QY("Validation failed")).text(result.second).exec();

  return false;
}

Tab::ValidationResult
Tab::copyControlsToStorageImpl(QModelIndex const &idx) {
  auto stdItem = m_chapterModel->itemFromIndex(idx);
  if (!stdItem)
    return { true, QString{} };

  if (!idx.parent().isValid())
    return copyEditionControlsToStorage(m_chapterModel->editionFromItem(stdItem));
  return copyChapterControlsToStorage(m_chapterModel->chapterFromItem(stdItem));
}

Tab::ValidationResult
Tab::copyChapterControlsToStorage(ChapterPtr const &chapter) {
  if (!chapter)
    return { true, QString{} };

  auto uid = uint64_t{};

  if (!ui->leChUid->text().isEmpty()) {
    auto ok = false;
    uid     = ui->leChUid->text().toULongLong(&ok);
    if (!ok)
      return { false, QY("The chapter UID must be a number if given.") };
  }

  if (uid)
    GetChild<KaxChapterUID>(*chapter).SetValue(uid);
  else
    DeleteChildren<KaxChapterUID>(*chapter);

  if (!ui->cbChFlagEnabled->isChecked())
    GetChild<KaxChapterFlagEnabled>(*chapter).SetValue(0);
  else
    DeleteChildren<KaxChapterFlagEnabled>(*chapter);

  if (ui->cbChFlagHidden->isChecked())
    GetChild<KaxChapterFlagHidden>(*chapter).SetValue(1);
  else
    DeleteChildren<KaxChapterFlagHidden>(*chapter);

  auto startTimecode = int64_t{};
  if (!parse_timecode(to_utf8(ui->leChStart->text()), startTimecode))
    return { false, QY("The start time could not be parsed: %1").arg(Q(timecode_parser_error)) };
  GetChild<KaxChapterTimeStart>(*chapter).SetValue(startTimecode);

  if (!ui->leChEnd->text().isEmpty()) {
    auto endTimecode = int64_t{};
    if (!parse_timecode(to_utf8(ui->leChEnd->text()), endTimecode))
      return { false, QY("The end time could not be parsed: %1").arg(Q(timecode_parser_error)) };

    if (endTimecode <= startTimecode)
      return { false, QY("The end time must be greater than the start timecode.") };

    GetChild<KaxChapterTimeEnd>(*chapter).SetValue(endTimecode);

  } else
    DeleteChildren<KaxChapterTimeEnd>(*chapter);

  if (!ui->leChSegmentUid->text().isEmpty()) {
    try {
      auto value = bitvalue_c{to_utf8(ui->leChSegmentUid->text())};
      GetChild<KaxChapterSegmentUID>(*chapter).CopyBuffer(value.data(), value.byte_size());

    } catch (mtx::bitvalue_parser_x const &ex) {
      return { false, QY("The segment UID could not be parsed: %1").arg(ex.what()) };
    }

  } else
    DeleteChildren<KaxChapterSegmentUID>(*chapter);

  if (!ui->leChSegmentEditionUid->text().isEmpty()) {
    auto ok = false;
    uid     = ui->leChSegmentEditionUid->text().toULongLong(&ok);
    if (!ok || !uid)
      return { false, QY("The segment edition UID must be a positive number if given.") };

    GetChild<KaxChapterSegmentEditionUID>(*chapter).SetValue(uid);
  }

  RemoveChildren<KaxChapterDisplay>(*chapter);
  for (auto row = 0, numRows = m_nameModel->rowCount(); row < numRows; ++row)
    chapter->PushElement(*m_nameModel->displayFromIndex(m_nameModel->index(row, 0)));

  return { true, QString{} };
}

Tab::ValidationResult
Tab::copyEditionControlsToStorage(EditionPtr const &edition) {
  if (!edition)
    return { true, QString{} };

  auto uid = uint64_t{};

  if (!ui->leEdUid->text().isEmpty()) {
    auto ok = false;
    uid     = ui->leEdUid->text().toULongLong(&ok);
    if (!ok)
      return { false, QY("The edition UID must be a number if given.") };
  }

  if (uid)
    GetChild<KaxEditionUID>(*edition).SetValue(uid);
  else
    DeleteChildren<KaxEditionUID>(*edition);

  if (ui->cbEdFlagDefault->isChecked())
    GetChild<KaxEditionFlagDefault>(*edition).SetValue(1);
  else
    DeleteChildren<KaxEditionFlagDefault>(*edition);

  if (ui->cbEdFlagHidden->isChecked())
    GetChild<KaxEditionFlagHidden>(*edition).SetValue(1);
  else
    DeleteChildren<KaxEditionFlagHidden>(*edition);

  if (ui->cbEdFlagOrdered->isChecked())
    GetChild<KaxEditionFlagOrdered>(*edition).SetValue(1);
  else
    DeleteChildren<KaxEditionFlagOrdered>(*edition);

  return { true, QString{} };
}

bool
Tab::setControlsFromStorage() {
  auto idx = Util::selectedRowIdx(ui->elements);
  return idx.isValid() ? setControlsFromStorage(idx) : true;
}

bool
Tab::setControlsFromStorage(QModelIndex const &idx) {
  auto stdItem = m_chapterModel->itemFromIndex(idx);
  if (!stdItem)
    return false;

  if (!idx.parent().isValid())
    return setEditionControlsFromStorage(m_chapterModel->editionFromItem(stdItem));
  return setChapterControlsFromStorage(m_chapterModel->chapterFromItem(stdItem));
}

bool
Tab::setChapterControlsFromStorage(ChapterPtr const &chapter) {
  if (!chapter)
    return true;

  auto uid               = FindChildValue<KaxChapterUID>(*chapter);
  auto end               = FindChild<KaxChapterTimeEnd>(*chapter);
  auto segmentEditionUid = FindChild<KaxChapterSegmentEditionUID>(*chapter);

  ui->lChapter->setText(m_chapterModel->chapterDisplayName(*chapter));
  ui->leChStart->setText(Q(format_timestamp(FindChildValue<KaxChapterTimeStart>(*chapter))));
  ui->leChEnd->setText(end ? Q(format_timestamp(end->GetValue())) : Q(""));
  ui->cbChFlagEnabled->setChecked(!!FindChildValue<KaxChapterFlagEnabled>(*chapter, 1));
  ui->cbChFlagHidden->setChecked(!!FindChildValue<KaxChapterFlagHidden>(*chapter));
  ui->leChUid->setText(uid ? QString::number(uid) : Q(""));
  ui->leChSegmentUid->setText(formatEbmlBinary(FindChild<KaxChapterSegmentUID>(*chapter)));
  ui->leChSegmentEditionUid->setText(segmentEditionUid ? QString::number(segmentEditionUid->GetValue()) : Q(""));

  auto nameSelectionModel        = ui->tvChNames->selectionModel();
  auto previouslySelectedNameIdx = nameSelectionModel->currentIndex();
  m_nameModel->populate(*chapter);
  enableNameWidgets(false);

  if (m_nameModel->rowCount()) {
    auto oldBlocked  = nameSelectionModel->blockSignals(true);
    auto rowToSelect = std::min(previouslySelectedNameIdx.isValid() ? previouslySelectedNameIdx.row() : 0, m_nameModel->rowCount());
    auto selection   = QItemSelection{ m_nameModel->index(rowToSelect, 0), m_nameModel->index(rowToSelect, m_nameModel->columnCount() - 1) };

    nameSelectionModel->select(selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current);

    setNameControlsFromStorage(m_nameModel->index(rowToSelect, 0));
    enableNameWidgets(true);

    nameSelectionModel->blockSignals(oldBlocked);
  }

  ui->pageContainer->setCurrentWidget(ui->chapterPage);

  return true;
}

bool
Tab::setEditionControlsFromStorage(EditionPtr const &edition) {
  if (!edition)
    return true;

  auto uid = FindChildValue<KaxEditionUID>(*edition);

  ui->leEdUid->setText(uid ? QString::number(uid) : Q(""));
  ui->cbEdFlagDefault->setChecked(!!FindChildValue<KaxEditionFlagDefault>(*edition));
  ui->cbEdFlagHidden->setChecked(!!FindChildValue<KaxEditionFlagHidden>(*edition));
  ui->cbEdFlagOrdered->setChecked(!!FindChildValue<KaxEditionFlagOrdered>(*edition));

  ui->pageContainer->setCurrentWidget(ui->editionPage);

  return true;
}

bool
Tab::handleChapterDeselection(QItemSelection const &deselected) {
  if (deselected.isEmpty())
    return true;

  auto indexes = deselected.at(0).indexes();
  return indexes.isEmpty() ? true : copyControlsToStorage(indexes.at(0));
}

void
Tab::chapterSelectionChanged(QItemSelection const &selected,
                             QItemSelection const &deselected) {
  auto selectedIdx = QModelIndex{};
  if (!selected.isEmpty()) {
    auto indexes = selected.at(0).indexes();
    if (!indexes.isEmpty())
      selectedIdx = indexes.at(0);
  }

  m_chapterModel->setSelectedIdx(selectedIdx);

  if (m_ignoreChapterSelectionChanges)
    return;

  if (!handleChapterDeselection(deselected))
    return;

  if (selectedIdx.isValid() && setControlsFromStorage(selectedIdx.sibling(selectedIdx.row(), 0)))
    return;

  ui->pageContainer->setCurrentWidget(ui->emptyPage);
}

bool
Tab::setNameControlsFromStorage(QModelIndex const &idx) {
  auto display = m_nameModel->displayFromIndex(idx);
  if (!display)
    return false;

  auto language = Q(FindChildValue<KaxChapterLanguage>(display, std::string{"eng"}));

  ui->leChName->setText(Q(GetChildValue<KaxChapterString>(display)));
  ui->cbChNameLanguage->setAdditionalItems(usedNameLanguages())
    .reInitializeIfNecessary()
    .setCurrentByData(language);
  ui->cbChNameCountry->setCurrentByData(Q(FindChildValue<KaxChapterCountry>(display)));

  resizeNameColumnsToContents();

  return true;
}

void
Tab::nameSelectionChanged(QItemSelection const &selected,
                          QItemSelection const &) {
  if (!selected.isEmpty()) {
    auto indexes = selected.at(0).indexes();
    if (!indexes.isEmpty() && setNameControlsFromStorage(indexes.at(0))) {
      enableNameWidgets(true);

      ui->leChName->selectAll();
      QTimer::singleShot(0, ui->leChName, SLOT(setFocus()));

      return;
    }
  }

  enableNameWidgets(false);
}

void
Tab::withSelectedName(std::function<void(QModelIndex const &, KaxChapterDisplay &)> const &worker) {
  auto selectedRows = ui->tvChNames->selectionModel()->selectedRows();
  if (selectedRows.isEmpty())
    return;

  auto idx     = selectedRows.at(0);
  auto display = m_nameModel->displayFromIndex(idx);
  if (display)
    worker(idx, *display);
}

void
Tab::chapterNameEdited(QString const &text) {
  withSelectedName([this, &text](QModelIndex const &idx, KaxChapterDisplay &display) {
    GetChild<KaxChapterString>(display).SetValueUTF8(to_utf8(text));
    m_nameModel->updateRow(idx.row());
  });
}

void
Tab::chapterNameLanguageChanged(int index) {
  if (0 > index)
    return;

  withSelectedName([this, index](QModelIndex const &idx, KaxChapterDisplay &display) {
    GetChild<KaxChapterLanguage>(display).SetValue(to_utf8(ui->cbChNameLanguage->itemData(index).toString()));
    m_nameModel->updateRow(idx.row());
  });
}

void
Tab::chapterNameCountryChanged(int index) {
  if (0 > index)
    return;

  withSelectedName([this, index](QModelIndex const &idx, KaxChapterDisplay &display) {
    if (0 == index)
      DeleteChildren<KaxChapterCountry>(display);
    else
      GetChild<KaxChapterCountry>(display).SetValue(to_utf8(ui->cbChNameCountry->currentData().toString()));
    m_nameModel->updateRow(idx.row());
  });
}

void
Tab::addChapterName() {
  m_nameModel->addNew();
}

void
Tab::removeChapterName() {
  auto idx = Util::selectedRowIdx(ui->tvChNames);
  if (!idx.isValid())
    return;

  m_nameModel->remove(idx);
}

void
Tab::enableNameWidgets(bool enable) {
  for (auto const &widget : m_nameWidgets)
    widget->setEnabled(enable);
}

void
Tab::expandAll() {
  expandCollapseAll(true);
}

void
Tab::collapseAll() {
  expandCollapseAll(false);
}

void
Tab::addEditionBefore() {
  addEdition(true);
}

void
Tab::addEditionAfter() {
  addEdition(false);
}

QModelIndex
Tab::addEdition(bool before) {
  auto edition     = std::make_shared<KaxEditionEntry>();
  auto selectedIdx = Util::selectedRowIdx(ui->elements);
  auto row         = 0;

  if (selectedIdx.isValid()) {
    while (selectedIdx.parent().isValid())
      selectedIdx = selectedIdx.parent();

    row = selectedIdx.row() + (before ? 0 : 1);
  }

  GetChild<KaxEditionUID>(*edition).SetValue(0);

  m_chapterModel->insertEdition(row, edition);

  emit numberOfEntriesChanged();

  return m_chapterModel->index(row, 0);
}

void
Tab::addEditionOrChapterAfter() {
  auto selectedIdx     = Util::selectedRowIdx(ui->elements);
  auto hasSelection    = selectedIdx.isValid();
  auto chapterSelected = hasSelection && selectedIdx.parent().isValid();

  if (!hasSelection)
    return;

  auto newEntryIdx = chapterSelected ? addChapter(false) : addEdition(false);
  selectChapterRow(newEntryIdx, false);
}

ChapterPtr
Tab::createEmptyChapter(int64_t startTime,
                        int chapterNumber,
                        boost::optional<QString> const &nameTemplate,
                        boost::optional<QString> const &language,
                        boost::optional<QString> const &country) {
  auto &cfg     = Util::Settings::get();
  auto chapter  = std::make_shared<KaxChapterAtom>();
  auto &display = GetChild<KaxChapterDisplay>(*chapter);
  auto name     = formatChapterName(nameTemplate ? *nameTemplate : cfg.m_chapterNameTemplate, chapterNumber, timestamp_c::ns(startTime));

  GetChild<KaxChapterUID>(*chapter).SetValue(0);
  GetChild<KaxChapterTimeStart>(*chapter).SetValue(startTime);
  GetChild<KaxChapterString>(display).SetValue(to_wide(name));
  GetChild<KaxChapterLanguage>(display).SetValue(to_utf8(language ? *language : cfg.m_defaultChapterLanguage));
  if ((country && !country->isEmpty()) || !cfg.m_defaultChapterCountry.isEmpty())
    GetChild<KaxChapterCountry>(display).SetValue(to_utf8((country && !country->isEmpty()) ? *country : cfg.m_defaultChapterCountry));

  return chapter;
}

void
Tab::addChapterBefore() {
  addChapter(true);
}

void
Tab::addChapterAfter() {
  addChapter(false);
}

QModelIndex
Tab::addChapter(bool before) {
  auto selectedIdx = Util::selectedRowIdx(ui->elements);
  if (!selectedIdx.isValid() || !selectedIdx.parent().isValid())
    return {};

  // TODO: Tab::addChapter: start time
  auto row     = selectedIdx.row() + (before ? 0 : 1);
  auto chapter = createEmptyChapter(0, row + 1);

  m_chapterModel->insertChapter(row, chapter, selectedIdx.parent());

  emit numberOfEntriesChanged();

  return m_chapterModel->index(row, 0, selectedIdx.parent());
}

void
Tab::addSubChapter() {
  auto selectedIdx = Util::selectedRowIdx(ui->elements);
  if (!selectedIdx.isValid())
    return;

  // TODO: Tab::addSubChapter: start time
  auto selectedItem = m_chapterModel->itemFromIndex(selectedIdx);
  auto chapter      = createEmptyChapter(0, (selectedItem ? selectedItem->rowCount() : 0) + 1);

  m_chapterModel->appendChapter(chapter, selectedIdx);
  expandCollapseAll(true, selectedIdx);

  emit numberOfEntriesChanged();
}

void
Tab::removeElement() {
  m_chapterModel->removeTree(Util::selectedRowIdx(ui->elements));
  emit numberOfEntriesChanged();
}

void
Tab::applyModificationToTimecodes(QStandardItem *item,
                                  std::function<int64_t(int64_t)> const &unaryOp) {
  if (!item)
    return;

  if (item->parent()) {
    auto chapter = m_chapterModel->chapterFromItem(item);
    if (chapter) {
      auto kStart = FindChild<KaxChapterTimeStart>(*chapter);
      auto kEnd   = FindChild<KaxChapterTimeEnd>(*chapter);

      if (kStart)
        kStart->SetValue(std::max<int64_t>(unaryOp(static_cast<int64_t>(kStart->GetValue())), 0));
      if (kEnd)
        kEnd->SetValue(std::max<int64_t>(unaryOp(static_cast<int64_t>(kEnd->GetValue())), 0));

      if (kStart || kEnd)
        m_chapterModel->updateRow(item->index());
    }
  }

  for (auto row = 0, numRows = item->rowCount(); row < numRows; ++row)
    applyModificationToTimecodes(item->child(row), unaryOp);
}

void
Tab::multiplyTimecodes(QStandardItem *item,
                       double factor) {
  applyModificationToTimecodes(item, [=](int64_t timecode) { return static_cast<int64_t>((timecode * factor) * 10.0 + 5) / 10ll; });
}

void
Tab::shiftTimecodes(QStandardItem *item,
                    int64_t delta) {
  applyModificationToTimecodes(item, [=](int64_t timecode) { return timecode + delta; });
}

void
Tab::constrictTimecodes(QStandardItem *item,
                        boost::optional<uint64_t> const &constrictStart,
                        boost::optional<uint64_t> const &constrictEnd) {
  if (!item)
    return;

  auto chapter = item->parent() ? m_chapterModel->chapterFromItem(item) : ChapterPtr{};
  if (!chapter) {
    for (auto row = 0, numRows = item->rowCount(); row < numRows; ++row)
      constrictTimecodes(item->child(row), {}, {});
    return;
  }

  auto kStart   = &GetChild<KaxChapterTimeStart>(*chapter);
  auto kEnd     = FindChild<KaxChapterTimeEnd>(*chapter);
  auto newStart = !constrictStart ? kStart->GetValue()
                : !constrictEnd   ? std::max(*constrictStart, kStart->GetValue())
                :                   std::min(*constrictEnd, std::max(*constrictStart, kStart->GetValue()));
  auto newEnd   = !kEnd           ? boost::optional<uint64_t>{}
                : !constrictEnd   ? std::max(newStart, kEnd->GetValue())
                :                   std::max(newStart, std::min(*constrictEnd, kEnd->GetValue()));

  kStart->SetValue(newStart);
  if (newEnd)
    GetChild<KaxChapterTimeEnd>(*chapter).SetValue(*newEnd);

  m_chapterModel->updateRow(item->index());

  for (auto row = 0, numRows = item->rowCount(); row < numRows; ++row)
    constrictTimecodes(item->child(row), newStart, newEnd);
}

std::pair<boost::optional<uint64_t>, boost::optional<uint64_t>>
Tab::expandTimecodes(QStandardItem *item) {
  if (!item)
    return {};

  auto chapter = item->parent() ? m_chapterModel->chapterFromItem(item) : ChapterPtr{};
  if (!chapter) {
    for (auto row = 0, numRows = item->rowCount(); row < numRows; ++row)
      expandTimecodes(item->child(row));
    return {};
  }

  auto kStart   = chapter ? FindChild<KaxChapterTimeStart>(*chapter)      : nullptr;
  auto kEnd     = chapter ? FindChild<KaxChapterTimeEnd>(*chapter)        : nullptr;
  auto newStart = kStart  ? boost::optional<uint64_t>{kStart->GetValue()} : boost::optional<uint64_t>{};
  auto newEnd   = kEnd    ? boost::optional<uint64_t>{kEnd->GetValue()}   : boost::optional<uint64_t>{};
  auto modified = false;

  for (auto row = 0, numRows = item->rowCount(); row < numRows; ++row) {
    auto startAndEnd = expandTimecodes(item->child(row));

    if (!newStart || (startAndEnd.first && (*startAndEnd.first < *newStart)))
      newStart = startAndEnd.first;

    if (!newEnd || (startAndEnd.second && (*startAndEnd.second > *newEnd)))
      newEnd = startAndEnd.second;
  }

  if (newStart && (!kStart || (kStart->GetValue() > *newStart))) {
    GetChild<KaxChapterTimeStart>(*chapter).SetValue(*newStart);
    modified = true;
  }

  if (newEnd && (!kEnd || (kEnd->GetValue() < *newEnd))) {
    GetChild<KaxChapterTimeEnd>(*chapter).SetValue(*newEnd);
    modified = true;
  }

  if (modified)
    m_chapterModel->updateRow(item->index());

  return std::make_pair(newStart, newEnd);
}

void
Tab::setLanguages(QStandardItem *item,
                  QString const &language) {
  if (!item)
    return;

  auto chapter = m_chapterModel->chapterFromItem(item);
  if (chapter)
    for (auto const &element : *chapter) {
      auto kDisplay = dynamic_cast<KaxChapterDisplay *>(element);
      if (kDisplay)
        GetChild<KaxChapterLanguage>(*kDisplay).SetValue(to_utf8(language));
    }

  for (auto row = 0, numRows = item->rowCount(); row < numRows; ++row)
    setLanguages(item->child(row), language);
}

void
Tab::setCountries(QStandardItem *item,
                  QString const &country) {
  if (!item)
    return;

  auto chapter = m_chapterModel->chapterFromItem(item);
  if (chapter)
    for (auto const &element : *chapter) {
      auto kDisplay = dynamic_cast<KaxChapterDisplay *>(element);
      if (!kDisplay)
        continue;

      if (country.isEmpty())
        DeleteChildren<KaxChapterCountry>(*kDisplay);
      else
        GetChild<KaxChapterCountry>(*kDisplay).SetValue(to_utf8(country));
    }

  for (auto row = 0, numRows = item->rowCount(); row < numRows; ++row)
    setCountries(item->child(row), country);
}

void
Tab::massModify() {
  if (!copyControlsToStorage())
    return;

  auto selectedIdx = Util::selectedRowIdx(ui->elements);
  auto item        = selectedIdx.isValid() ? m_chapterModel->itemFromIndex(selectedIdx) : m_chapterModel->invisibleRootItem();

  MassModificationDialog dlg{this, selectedIdx.isValid(), usedNameLanguages()};
  if (!dlg.exec())
    return;

  auto actions = dlg.actions();

  if (actions & MassModificationDialog::Shift)
    shiftTimecodes(item, dlg.shiftBy());

  if (actions & MassModificationDialog::Multiply)
    multiplyTimecodes(item, dlg.multiplyBy());

  if (actions & MassModificationDialog::Constrict)
    constrictTimecodes(item, {}, {});

  if (actions & MassModificationDialog::Expand)
    expandTimecodes(item);

  if (actions & MassModificationDialog::SetLanguage)
    setLanguages(item, dlg.language());

  if (actions & MassModificationDialog::SetCountry)
    setCountries(item, dlg.country());

  if (actions & MassModificationDialog::Sort)
    item->sortChildren(1);

  setControlsFromStorage();
}

void
Tab::duplicateElement() {
  auto selectedIdx   = Util::selectedRowIdx(ui->elements);
  auto newElementIdx = m_chapterModel->duplicateTree(selectedIdx);

  if (newElementIdx.isValid())
    expandCollapseAll(true, newElementIdx);

  emit numberOfEntriesChanged();
}

QString
Tab::formatChapterName(QString const &nameTemplate,
                       int chapterNumber,
                       timestamp_c const &startTimecode)
  const {
  return Q(format_chapter_name_template(to_utf8(nameTemplate), chapterNumber, startTimecode));
}

void
Tab::generateSubChapters() {
  auto selectedIdx = Util::selectedRowIdx(ui->elements);
  if (!selectedIdx.isValid())
    return;

  if (!copyControlsToStorage())
    return;

  auto selectedItem    = m_chapterModel->itemFromIndex(selectedIdx);
  auto selectedChapter = m_chapterModel->chapterFromItem(selectedItem);
  auto maxEndTimecode  = selectedChapter ? FindChildValue<KaxChapterTimeStart>(*selectedChapter, 0ull) : 0ull;
  auto numRows         = selectedItem->rowCount();

  for (auto row = 0; row < numRows; ++row) {
    auto chapter = m_chapterModel->chapterFromItem(selectedItem->child(row));
    if (chapter)
      maxEndTimecode = std::max(maxEndTimecode, std::max(FindChildValue<KaxChapterTimeStart>(*chapter, 0ull), FindChildValue<KaxChapterTimeEnd>(*chapter, 0ull)));
  }

  GenerateSubChaptersParametersDialog dlg{this, numRows + 1, maxEndTimecode, usedNameLanguages()};
  if (!dlg.exec())
    return;

  auto toCreate      = dlg.numberOfEntries();
  auto chapterNumber = dlg.firstChapterNumber();
  auto timecode      = dlg.startTimecode();
  auto duration      = dlg.durationInNs();
  auto nameTemplate  = dlg.nameTemplate();
  auto language      = dlg.language();
  auto country       = dlg.country();

  while (toCreate > 0) {
    auto chapter = createEmptyChapter(timecode, chapterNumber, nameTemplate, language, country);
    timecode    += duration;

    ++chapterNumber;
    --toCreate;

    m_chapterModel->appendChapter(chapter, selectedIdx);
  }

  expandCollapseAll(true, selectedIdx);
  resizeNameColumnsToContents();

  emit numberOfEntriesChanged();
}

bool
Tab::changeChapterName(QModelIndex const &parentIdx,
                       int row,
                       int chapterNumber,
                       QString const &nameTemplate,
                       RenumberSubChaptersParametersDialog::NameMatch nameMatchingMode,
                       QString const &languageOfNamesToReplace,
                       bool skipHidden) {
  auto idx     = m_chapterModel->index(row, 0, parentIdx);
  auto item    = m_chapterModel->itemFromIndex(idx);
  auto chapter = m_chapterModel->chapterFromItem(item);

  if (!chapter)
    return false;

  if (skipHidden) {
    auto flagHidden = FindChild<KaxChapterFlagHidden>(*chapter);
    if (flagHidden && flagHidden->GetValue())
      return false;
  }

  auto startTimecode = FindChildValue<KaxChapterTimeStart>(*chapter);
  auto name          = to_wide(formatChapterName(nameTemplate, chapterNumber, timestamp_c::ns(startTimecode)));

  if (RenumberSubChaptersParametersDialog::NameMatch::First == nameMatchingMode) {
    GetChild<KaxChapterString>(GetChild<KaxChapterDisplay>(*chapter)).SetValue(name);
    m_chapterModel->updateRow(idx);

    return true;
  }

  for (auto const &element : *chapter) {
    auto kDisplay = dynamic_cast<KaxChapterDisplay *>(element);
    if (!kDisplay)
      continue;

    auto language = FindChildValue<KaxChapterLanguage>(kDisplay, std::string{"eng"});
    if (   (RenumberSubChaptersParametersDialog::NameMatch::All == nameMatchingMode)
        || (Q(language)                                         == languageOfNamesToReplace))
      GetChild<KaxChapterString>(*kDisplay).SetValue(name);
  }

  m_chapterModel->updateRow(idx);

  return true;
}

void
Tab::renumberSubChapters() {
  auto selectedIdx = Util::selectedRowIdx(ui->elements);
  if (!selectedIdx.isValid())
    return;

  if (!copyControlsToStorage())
    return;

  auto selectedItem    = m_chapterModel->itemFromIndex(selectedIdx);
  auto selectedChapter = m_chapterModel->chapterFromItem(selectedItem);
  auto numRows         = selectedItem->rowCount();
  auto chapterTitles   = QStringList{};
  auto firstName       = QString{};

  for (auto row = 0; row < numRows; ++row) {
    auto chapter = m_chapterModel->chapterFromItem(selectedItem->child(row));
    if (!chapter)
      continue;

    auto start = GetChild<KaxChapterTimeStart>(*chapter).GetValue();
    auto end   = FindChild<KaxChapterTimeEnd>(*chapter);
    auto name  = ChapterModel::chapterDisplayName(*chapter);

    if (firstName.isEmpty())
      firstName = name;

    if (end)
      chapterTitles << Q("%1 (%2 – %3)").arg(name).arg(Q(format_timestamp(start))).arg(Q(format_timestamp(end->GetValue())));
    else
      chapterTitles << Q("%1 (%2)").arg(name).arg(Q(format_timestamp(start)));
  }

  auto matches       = QRegularExpression{Q("(\\d+)$")}.match(firstName);
  auto firstNumber   = matches.hasMatch() ? matches.captured(0).toInt() : 1;
  auto usedLanguages = usedNameLanguages(selectedItem);

  RenumberSubChaptersParametersDialog dlg{this, firstNumber, chapterTitles, usedLanguages};
  if (!dlg.exec())
    return;

  auto row               = dlg.firstEntryToRenumber();
  auto toRenumber        = dlg.numberOfEntries() ? dlg.numberOfEntries() : numRows;
  auto chapterNumber     = dlg.firstChapterNumber();
  auto nameTemplate      = dlg.nameTemplate();
  auto nameMatchingMode  = dlg.nameMatchingMode();
  auto languageToReplace = dlg.languageOfNamesToReplace();
  auto skipHidden        = dlg.skipHidden();

  while ((row < numRows) && (0 < toRenumber)) {
    auto renumbered = changeChapterName(selectedIdx, row, chapterNumber, nameTemplate, nameMatchingMode, languageToReplace, skipHidden);

    if (renumbered)
      ++chapterNumber;
    ++row;
  }

  resizeNameColumnsToContents();
}

void
Tab::expandCollapseAll(bool expand,
                       QModelIndex const &parentIdx) {
  if (parentIdx.isValid())
    ui->elements->setExpanded(parentIdx, expand);

  for (auto row = 0, numRows = m_chapterModel->rowCount(parentIdx); row < numRows; ++row)
    expandCollapseAll(expand, m_chapterModel->index(row, 0, parentIdx));
}

void
Tab::expandInsertedElements(QModelIndex const &parentIdx,
                            int,
                            int) {
  expandCollapseAll(true, parentIdx);
}

QString
Tab::formatEbmlBinary(EbmlBinary *binary) {
  auto value = std::string{};
  auto data  = static_cast<unsigned char const *>(binary ? binary->GetBuffer() : nullptr);

  if (data)
    for (auto end = data + binary->GetSize(); data < end; ++data)
      value += (boost::format("%|1$02x|") % static_cast<unsigned int>(*data)).str();

  return Q(value);
}

void
Tab::showChapterContextMenu(QPoint const &pos) {
  auto selectedIdx     = Util::selectedRowIdx(ui->elements);
  auto hasSelection    = selectedIdx.isValid();
  auto chapterSelected = hasSelection && selectedIdx.parent().isValid();
  auto hasEntries      = !!m_chapterModel->rowCount();
  auto hasSubEntries   = selectedIdx.isValid() ? !!m_chapterModel->rowCount(selectedIdx) : false;

  m_addChapterBeforeAction->setEnabled(chapterSelected);
  m_addChapterAfterAction->setEnabled(chapterSelected);
  m_addSubChapterAction->setEnabled(hasSelection);
  m_generateSubChaptersAction->setEnabled(hasSelection);
  m_renumberSubChaptersAction->setEnabled(hasSelection && hasSubEntries);
  m_removeElementAction->setEnabled(hasSelection);
  m_duplicateAction->setEnabled(hasSelection);
  m_expandAllAction->setEnabled(hasEntries);
  m_collapseAllAction->setEnabled(hasEntries);

  QMenu menu{this};

  menu.addAction(m_addEditionBeforeAction);
  menu.addAction(m_addEditionAfterAction);
  menu.addSeparator();
  menu.addAction(m_addChapterBeforeAction);
  menu.addAction(m_addChapterAfterAction);
  menu.addAction(m_addSubChapterAction);
  menu.addAction(m_generateSubChaptersAction);
  menu.addSeparator();
  menu.addAction(m_duplicateAction);
  menu.addSeparator();
  menu.addAction(m_removeElementAction);
  menu.addSeparator();
  menu.addAction(m_renumberSubChaptersAction);
  menu.addAction(m_massModificationAction);
  menu.addSeparator();
  menu.addAction(m_expandAllAction);
  menu.addAction(m_collapseAllAction);

  menu.exec(ui->elements->viewport()->mapToGlobal(pos));
}

bool
Tab::isSourceMatroska()
  const {
  return !!m_analyzer;
}

bool
Tab::hasChapters()
  const {
  for (auto idx = 0, numEditions = m_chapterModel->rowCount(); idx < numEditions; ++idx)
    if (m_chapterModel->item(idx)->rowCount())
      return true;
  return false;
}

QString
Tab::currentState()
  const {
  auto chapters = m_chapterModel->allChapters();
  return chapters ? Q(ebml_dumper_c::dump_to_string(chapters.get(), static_cast<ebml_dumper_c::dump_style_e>(ebml_dumper_c::style_with_values | ebml_dumper_c::style_with_indexes))) : QString{};
}

bool
Tab::hasBeenModified()
  const {
  return currentState() != m_savedState;
}

bool
Tab::focusNextChapterName() {
  auto selectedRows = ui->tvChNames->selectionModel()->selectedRows();
  if (selectedRows.isEmpty())
    return false;

  auto nextRow = selectedRows.at(0).row() + 1;
  if (nextRow >= m_nameModel->rowCount())
    return false;

  Util::selectRow(ui->tvChNames, nextRow);

  return true;
}

bool
Tab::focusNextChapterAtom(FocusElementType toFocus) {
  auto doSelect = [this, toFocus](QModelIndex const &idx) -> bool {
    Util::selectRow(ui->elements, idx.row(), idx.parent());

    auto lineEdit = FocusChapterStartTime == toFocus ? ui->leChStart : ui->leChName;
    lineEdit->selectAll();
    lineEdit->setFocus();

    return true;
  };

  auto selectedRows = ui->elements->selectionModel()->selectedRows();
  if (selectedRows.isEmpty())
    return false;

  auto selectedIdx  = selectedRows.at(0);
  selectedIdx       = selectedIdx.sibling(selectedIdx.row(), 0);
  auto selectedItem = m_chapterModel->itemFromIndex(selectedIdx);

  if (selectedItem->rowCount())
    return doSelect(selectedIdx.child(0, 0));

  auto parentIdx  = selectedIdx.parent();
  auto parentItem = m_chapterModel->itemFromIndex(parentIdx);
  auto nextRow    = selectedIdx.row() + 1;

  if (nextRow < parentItem->rowCount())
    return doSelect(parentIdx.child(nextRow, 0));

  while (parentIdx.parent().isValid()) {
    nextRow    = parentIdx.row() + 1;
    parentIdx  = parentIdx.parent();
    parentItem = m_chapterModel->itemFromIndex(parentIdx);

    if (nextRow < parentItem->rowCount())
      return doSelect(parentIdx.child(nextRow, 0));
  }

  auto numEditions = m_chapterModel->rowCount();
  auto editionIdx  = parentIdx.sibling((parentIdx.row() + 1) % numEditions, 0);

  while (numEditions) {
    if (m_chapterModel->itemFromIndex(editionIdx)->rowCount())
      return doSelect(editionIdx.child(0, 0));

    editionIdx = editionIdx.sibling((editionIdx.row() + 1) % numEditions, 0);
  }

  return false;
}

void
Tab::focusNextChapterElement(bool keepSameElement) {
  if (!copyControlsToStorage())
    return;

  if (QObject::sender() == ui->leChName) {
    if (focusNextChapterName())
      return;

    focusNextChapterAtom(keepSameElement ? FocusChapterName : FocusChapterStartTime);
    return;
  }

  if (!keepSameElement) {
    ui->leChName->selectAll();
    ui->leChName->setFocus();
    return;
  }

  focusNextChapterAtom(FocusChapterStartTime);
}

void
Tab::focusOtherControlInNextChapterElement() {
  focusNextChapterElement(false);
}

void
Tab::focusSameControlInNextChapterElement() {
  focusNextChapterElement(true);
}

void
Tab::closeTab() {
  emit removeThisTab();
}

void
Tab::addSegmentUIDFromFile() {
  Util::addSegmentUIDFromFileToLineEdit(*this, *ui->leChSegmentUid, false);
}

QStringList
Tab::usedNameLanguages(QStandardItem *rootItem) {
  if (!rootItem)
    rootItem = m_chapterModel->invisibleRootItem();

  auto names = QSet<QString>{};

  std::function<void(QStandardItem *)> collector = [&](QStandardItem *currentItem) {
    if (!currentItem)
      return;

    auto chapter = m_chapterModel->chapterFromItem(currentItem);
    if (chapter)
      for (auto const &element : *chapter) {
        auto kDisplay = dynamic_cast<KaxChapterDisplay *>(element);
        if (!kDisplay)
          continue;

        auto kLanguage = FindChild<KaxChapterLanguage>(*kDisplay);
        if (kLanguage)
          names << Q(*kLanguage);
      }

    for (auto row = 0, numRows = currentItem->rowCount(); row < numRows; ++row)
      collector(currentItem->child(row));
  };

  collector(rootItem);

  return names.toList();
}

}}}
