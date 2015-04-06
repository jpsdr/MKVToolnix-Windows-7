#include "common/common_pch.h"

#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>

#include <matroska/KaxSemantic.h>

#include "common/bitvalue.h"
#include "common/chapters/chapters.h"
#include "common/ebml.h"
#include "common/mm_io_x.h"
#include "common/qt.h"
#include "common/segmentinfo.h"
#include "common/segment_tracks.h"
#include "common/strings/formatting.h"
#include "common/strings/parsing.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/forms/chapter_editor/tab.h"
#include "mkvtoolnix-gui/chapter_editor/name_model.h"
#include "mkvtoolnix-gui/chapter_editor/tab.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/util/util.h"

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
  , m_sortSubtreeAction{new QAction{this}}
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
  ui->elements->setModel(m_chapterModel);
  ui->tvChNames->setModel(m_nameModel);

  ui->cbChNameLanguage->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  ui->cbChNameCountry ->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

  auto &languageDescriptions = App::getIso639LanguageDescriptions();
  auto &languageCodes        = App::getIso639_2LanguageCodes();
  auto &countryCodes         = App::getIso3166_1Alpha2CountryCodes();

  for (auto idx = 0, count = languageDescriptions.count(); idx < count; ++idx)
    ui->cbChNameLanguage->addItem(languageDescriptions[idx], languageCodes[idx]);

  ui->cbChNameCountry->addItem(Q(""));
  ui->cbChNameCountry->addItems(countryCodes);

  m_nameWidgets << ui->pbChRemoveName
                << ui->lChName         << ui->leChName
                << ui->lChNameLanguage << ui->cbChNameLanguage
                << ui->lChNameCountry  << ui->cbChNameCountry;

  connect(ui->elements,                    &ChapterTreeView::customContextMenuRequested,                           this, &Tab::showChapterContextMenu);
  connect(ui->elements->selectionModel(),  &QItemSelectionModel::selectionChanged,                                 this, &Tab::chapterSelectionChanged);
  connect(ui->tvChNames->selectionModel(), &QItemSelectionModel::selectionChanged,                                 this, &Tab::nameSelectionChanged);
  connect(ui->leChName,                    &QLineEdit::textEdited,                                                 this, &Tab::chapterNameEdited);
  connect(ui->cbChNameLanguage,            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &Tab::chapterNameLanguageChanged);
  connect(ui->cbChNameCountry,             static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &Tab::chapterNameCountryChanged);
  connect(ui->pbChAddName,                 &QPushButton::clicked,                                                  this, &Tab::addChapterName);
  connect(ui->pbChRemoveName,              &QPushButton::clicked,                                                  this, &Tab::removeChapterName);

  connect(m_expandAllAction,               &QAction::triggered,                                                    this, &Tab::expandAll);
  connect(m_collapseAllAction,             &QAction::triggered,                                                    this, &Tab::collapseAll);
  connect(m_addEditionBeforeAction,        &QAction::triggered,                                                    this, &Tab::addEditionBefore);
  connect(m_addEditionAfterAction,         &QAction::triggered,                                                    this, &Tab::addEditionAfter);
  connect(m_addChapterBeforeAction,        &QAction::triggered,                                                    this, &Tab::addChapterBefore);
  connect(m_addChapterAfterAction,         &QAction::triggered,                                                    this, &Tab::addChapterAfter);
  connect(m_addSubChapterAction,           &QAction::triggered,                                                    this, &Tab::addSubChapter);
  connect(m_removeElementAction,           &QAction::triggered,                                                    this, &Tab::removeElement);
  connect(m_sortSubtreeAction,             &QAction::triggered,                                                    this, &Tab::sortSubtree);
}

void
Tab::retranslateUi() {
  ui->retranslateUi(this);

  if (!m_fileName.isEmpty()) {
    auto info = QFileInfo{m_fileName};
    ui->fileName->setText(info.fileName());
    ui->directory->setText(info.path());

  } else {
    ui->fileName->setText(QY("<unsaved file>"));
    ui->directory->setText(Q(""));

  }

  m_expandAllAction->setText(QY("&Expand all"));
  m_collapseAllAction->setText(QY("&Collapse all"));
  m_addEditionBeforeAction->setText(QY("Add new e&dition before"));
  m_addEditionAfterAction->setText(QY("Add new ed&ition after"));
  m_addChapterBeforeAction->setText(QY("Add new c&hapter before"));
  m_addChapterAfterAction->setText(QY("Add new ch&apter after"));
  m_addSubChapterAction->setText(QY("Add new &sub-chapter inside"));
  m_removeElementAction->setText(QY("&Remove selected edition or chapter"));
  m_sortSubtreeAction->setText(QY("Sor&t elements by their start time"));

  m_chapterModel->retranslateUi();
  m_nameModel->retranslateUi();

  resizeChapterColumnsToContents();
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
Tab::getTitle()
  const {
  if (m_fileName.isEmpty())
    return QY("<unsaved file>");
  return QFileInfo{m_fileName}.fileName();
}

QString const &
Tab::getFileName()
  const {
  return m_fileName;
}

void
Tab::newFile() {
}

void
Tab::resetData() {
  m_analyzer.reset();
  m_nameModel->reset();
  m_chapterModel->reset();
}

ChaptersPtr
Tab::loadFromMatroskaFile() {
  m_analyzer = std::make_unique<QtKaxAnalyzer>(this, m_fileName);

  if (!m_analyzer->process(kax_analyzer_c::parse_mode_fast)) {
    QMessageBox::critical(this, QY("File parsing failed"), QY("The file you tried to open (%1) could not be read successfully.").arg(m_fileName));
    emit removeThisTab();
    return {};
  }

  auto idx = m_analyzer->find(KaxChapters::ClassInfos.GlobalId);
  if (-1 == idx) {
    QMessageBox::critical(this, QY("File parsing failed"), QY("The file you tried to open (%1) does not contain any chapters.").arg(m_fileName));
    emit removeThisTab();
    return {};
  }

  auto chapters = m_analyzer->read_element(idx);
  if (!chapters) {
    QMessageBox::critical(this, QY("File parsing failed"), QY("The file you tried to open (%1) could not be read successfully.").arg(m_fileName));
    emit removeThisTab();
  }

  return std::static_pointer_cast<KaxChapters>(chapters);
}

ChaptersPtr
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
    auto message = QY("The file you tried to open (%1) is not recognized as either a valid Matroska/WebM or a valid chapter file.").arg(m_fileName);
    if (!error.isEmpty())
      message = Q("%1 %2").arg(message).arg(QY("Error message from the parser: %1").arg(error));

    QMessageBox::critical(this, QY("File parsing failed"), message);
    emit removeThisTab();

  } else if (isSimpleFormat) {
    QMessageBox::warning(this, QY("Simple chapter file format"), QY("The file you tried to open (%1) is a simple chapter file format. This can only be read but not written. "
                                                                    "You will have to save the file to a Matroska/WebM or an XML chapter file.").arg(m_fileName));
    m_fileName.clear();
    emit titleChanged(getTitle());
  }

  return chapters;
}

void
Tab::load() {
  resetData();

  auto chapters = kax_analyzer_c::probe(to_utf8(m_fileName)) ? loadFromMatroskaFile() : loadFromChapterFile();

  if (!chapters)
    return;

  if (!m_fileName.isEmpty())
    m_fileModificationTime = QFileInfo{m_fileName}.lastModified();

  m_chapterModel->populate(*chapters);
  expandAll();
  resizeChapterColumnsToContents();

  MainWindow::get()->setStatusBarMessage(Q("yay loaded %1").arg(chapters->ListSize()));
}

// void
// Tab::save() {
//   auto segmentinfoModified = false;
//   auto tracksModified      = false;

//   for (auto const &page : m_chapterModel->getTopLevelPages()) {
//     if (!page->hasBeenModified())
//       continue;

//     if (page == m_segmentinfoPage)
//       segmentinfoModified = true;
//     else
//       tracksModified      = true;
//   }

//   if (!segmentinfoModified && !tracksModified) {
//     QMessageBox::information(this, QY("File has not been modified"), QY("The chapter values have not been modified. There is nothing to save."));
//     return;
//   }

//   auto pageIdx = m_chapterModel->validate();
//   if (pageIdx.isValid()) {
//     reportValidationFailure(false, pageIdx);
//     return;
//   }

//   if (QFileInfo{m_fileName}.lastModified() != m_fileModificationTime) {
//     QMessageBox::critical(this, QY("File has been modified"),
//                           QY("The file has been changed by another program since it was read by the chapter editor. Therefore you have to re-load it. Unfortunately this means that all of your changes will be lost."));
//     return;
//   }

//   doModifications();

//   if (segmentinfoModified && m_eSegmentInfo) {
//     auto result = m_analyzer->update_element(m_eSegmentInfo, true);
//     if (kax_analyzer_c::uer_success != result)
//       displayUpdateElementResult(result, QY("Saving the modified segment information chapter failed."));
//   }

//   if (tracksModified && m_eTracks) {
//     auto result = m_analyzer->update_element(m_eTracks, true);
//     if (kax_analyzer_c::uer_success != result)
//       displayUpdateElementResult(result, QY("Saving the modified track chapters failed."));
//   }

//   load();

//   MainWindow::get()->setStatusBarMessage(QY("The file has been saved successfully."));
// }

void
Tab::selectChapterRow(QModelIndex const &idx,
                      bool ignoreSelectionChanges) {
  auto selection = QItemSelection{idx.sibling(idx.row(), 0), idx.sibling(idx.row(), 2)};

  m_ignoreChapterSelectionChanges = ignoreSelectionChanges;
  ui->elements->selectionModel()->setCurrentIndex(idx.sibling(idx.row(), 0), QItemSelectionModel::ClearAndSelect);
  ui->elements->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
  m_ignoreChapterSelectionChanges = false;
}

bool
Tab::copyControlsToStorage(QModelIndex const &idx) {
  auto result = copyControlsToStorageImpl(idx);
  if (result.first) {
    m_chapterModel->updateRow(idx);
    return true;
  }

  selectChapterRow(idx, true);

  QMessageBox::critical(this, QY("Validation failed"), result.second);

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
  // TODO: Tab::copyChapterControlsToStorage: propagate start time to parent?
  if (!chapter)
    return { true, QString{} };

  auto ok  = false;
  auto uid = ui->leChUid->text().toULongLong(&ok);
  if (!ok || !uid)
    return { false, QY("The chapter UID must be positive number.") };

  GetChild<KaxChapterUID>(*chapter).SetValue(uid);

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
    uid = ui->leChSegmentEditionUid->text().toULongLong(&ok);
    if (!ok || !uid)
      return { false, QY("The segment edition UID must be positive number if it is given.") };

    GetChild<KaxChapterSegmentEditionUID>(*chapter).SetValue(uid);
  }

  return { true, QString{} };
}

Tab::ValidationResult
Tab::copyEditionControlsToStorage(EditionPtr const &edition) {
  if (!edition)
    return { true, QString{} };

  auto ok  = false;
  auto uid = ui->leEdUid->text().toULongLong(&ok);
  if (!ok || !uid)
    return { false, QY("The edition UID must be positive number.") };

  GetChild<KaxEditionUID>(*edition).SetValue(uid);

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
    return false;

  auto end               = FindChild<KaxChapterTimeEnd>(*chapter);
  auto segmentEditionUid = FindChild<KaxChapterSegmentEditionUID>(*chapter);

  ui->lChapter->setText(m_chapterModel->chapterDisplayName(*chapter));
  ui->leChStart->setText(Q(format_timecode(FindChildValue<KaxChapterTimeStart>(*chapter))));
  ui->leChEnd->setText(end ? Q(format_timecode(end->GetValue())) : Q(""));
  ui->cbChFlagEnabled->setChecked(!!FindChildValue<KaxChapterFlagEnabled>(*chapter, 1));
  ui->cbChFlagHidden->setChecked(!!FindChildValue<KaxChapterFlagHidden>(*chapter));
  ui->leChUid->setText(QString::number(FindChildValue<KaxChapterUID>(*chapter)));
  ui->leChSegmentUid->setText(formatEbmlBinary(FindChild<KaxChapterSegmentUID>(*chapter)));
  ui->leChSegmentEditionUid->setText(segmentEditionUid ? QString::number(segmentEditionUid->GetValue()) : Q(""));

  m_nameModel->populate(*chapter);
  enableNameWidgets(false);

  ui->pageContainer->setCurrentWidget(ui->chapterPage);

  return true;
}

bool
Tab::setEditionControlsFromStorage(EditionPtr const &edition) {
  if (!edition)
    return false;

  ui->leEdUid->setText(QString::number(FindChildValue<KaxEditionUID>(*edition)));
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
  if (m_ignoreChapterSelectionChanges)
    return;

  if (!handleChapterDeselection(deselected))
    return;

  if (!selected.isEmpty()) {
    auto indexes = selected.at(0).indexes();
    if (!indexes.isEmpty()) {
      auto idx = indexes.at(0);
      if (setControlsFromStorage(idx.sibling(idx.row(), 0)))
        return;
    }
  }

  ui->pageContainer->setCurrentWidget(ui->emptyPage);
}

bool
Tab::setNameControlsFromStorage(QModelIndex const &idx) {
  auto display = m_nameModel->displayFromIndex(idx);
  if (!display)
    return false;

  auto country = FindChild<KaxChapterCountry>(display);

  ui->leChName->setText(Q(GetChildValue<KaxChapterString>(display)));
  Util::setComboBoxTextByData(ui->cbChNameLanguage, Q(FindChildValue<KaxChapterLanguage>(display, std::string{"eng"})));
  ui->cbChNameCountry->setCurrentText(country ? Q(country->GetValue()) : Q(""));

  resizeNameColumnsToContents();

  ui->leChName->setFocus();
  ui->leChName->selectAll();

  return true;
}

void
Tab::nameSelectionChanged(QItemSelection const &selected,
                          QItemSelection const &) {
  if (!selected.isEmpty()) {
    auto indexes = selected.at(0).indexes();
    if (!indexes.isEmpty() && setNameControlsFromStorage(indexes.at(0))) {
      enableNameWidgets(true);
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
      GetChild<KaxChapterCountry>(display).SetValue(to_utf8(ui->cbChNameCountry->currentText()));
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

void
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
}

ChapterPtr
Tab::createEmptyChapter(int64_t startTime) {
  auto chapter  = std::make_shared<KaxChapterAtom>();
  auto &display = GetChild<KaxChapterDisplay>(*chapter);

  GetChild<KaxChapterUID>(*chapter).SetValue(0);
  GetChild<KaxChapterTimeStart>(*chapter).SetValue(startTime);
  GetChild<KaxChapterString>(display).SetValueUTF8(Y("<unnamed>"));
  GetChild<KaxChapterLanguage>(display).SetValue(Y("und"));

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

void
Tab::addChapter(bool before) {
  auto selectedIdx = Util::selectedRowIdx(ui->elements);
  if (!selectedIdx.isValid() || !selectedIdx.parent().isValid())
    return;

  // TODO: Tab::addChapter: start time
  auto chapter = createEmptyChapter(0);
  auto row     = selectedIdx.row() + (before ? 0 : 1);

  m_chapterModel->insertChapter(row, chapter, selectedIdx.parent());
}

void
Tab::addSubChapter() {
  auto selectedIdx = Util::selectedRowIdx(ui->elements);
  if (!selectedIdx.isValid())
    return;

  // TODO: Tab::addSubChapter: start time
  auto chapter = createEmptyChapter(0);
  m_chapterModel->appendChapter(chapter, selectedIdx);
  expandCollapseAll(true, selectedIdx);
}

void
Tab::removeElement() {
  auto selectedIdx = Util::selectedRowIdx(ui->elements);
  if (selectedIdx.isValid())
    m_chapterModel->removeRow(selectedIdx.row(), selectedIdx.parent());
}

void
Tab::sortSubtree() {
  auto selectedIdx = Util::selectedRowIdx(ui->elements);
  auto parentItem  = !selectedIdx.isValid() ? m_chapterModel->invisibleRootItem() : m_chapterModel->itemFromIndex(selectedIdx);
  parentItem->sortChildren(1);
}

void
Tab::expandCollapseAll(bool expand,
                       QModelIndex const &parentIdx) {
  if (parentIdx.isValid())
    ui->elements->setExpanded(parentIdx, expand);

  for (auto row = 0, numRows = m_chapterModel->rowCount(parentIdx); row < numRows; ++row)
    expandCollapseAll(expand, m_chapterModel->index(row, 0, parentIdx));
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

  m_expandAllAction->setEnabled(hasEntries);
  m_collapseAllAction->setEnabled(hasEntries);

  m_addChapterBeforeAction->setEnabled(chapterSelected);
  m_addChapterAfterAction->setEnabled(chapterSelected);
  m_addSubChapterAction->setEnabled(hasSelection);
  m_removeElementAction->setEnabled(hasSelection);

  QMenu menu{this};

  menu.addAction(m_addEditionBeforeAction);
  menu.addAction(m_addEditionAfterAction);
  menu.addSeparator();
  menu.addAction(m_addChapterBeforeAction);
  menu.addAction(m_addChapterAfterAction);
  menu.addAction(m_addSubChapterAction);
  menu.addSeparator();
  menu.addAction(m_removeElementAction);
  menu.addSeparator();
  menu.addAction(m_sortSubtreeAction);
  menu.addSeparator();
  menu.addAction(m_expandAllAction);
  menu.addAction(m_collapseAllAction);

  menu.exec(ui->elements->viewport()->mapToGlobal(pos));
}

}}}
