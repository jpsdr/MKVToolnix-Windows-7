#include "common/common_pch.h"

#include <QFileInfo>
#include <QMessageBox>

#include <matroska/KaxSemantic.h>

#include "common/chapters/chapters.h"
#include "common/ebml.h"
#include "common/mm_io_x.h"
#include "common/qt.h"
#include "common/segmentinfo.h"
#include "common/segment_tracks.h"
#include "common/strings/formatting.h"
#include "mkvtoolnix-gui/forms/chapter_editor/tab.h"
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
  , m_expandAllAction{new QAction{this}}
  , m_collapseAllAction{new QAction{this}}
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
  ui->elements->addAction(m_expandAllAction);
  ui->elements->addAction(m_collapseAllAction);

  connect(ui->elements->selectionModel(),  &QItemSelectionModel::selectionChanged, this, &Tab::chapterSelectionChanged);
  // connect(ui->tvChNames->selectionModel(), &QItemSelectionModel::selectionChanged, this, &Tab::nameSelectionChanged);
  // connect(ui->teChName,                    &QLineEdit::textEdited,                 this, &Tab::chapterNameEdited);
  // connect(ui->cbChNameLanguage,            &QLineEdit::currentIndexChanged,        this, &Tab::chapterNameLanguageChanged);
  // connect(ui->cbChNameCountry,             &QLineEdit::currentIndexChanged,        this, &Tab::chapterNameCountryChanged);

  connect(m_expandAllAction,               &QAction::triggered,                    this, &Tab::expandAll);
  connect(m_collapseAllAction,             &QAction::triggered,                    this, &Tab::collapseAll);

  m_nameWidgets << ui->pbChRemoveName
                << ui->lChName         << ui->leChName
                << ui->lChNameLanguage << ui->cbChNameLanguage
                << ui->lChNameCountry  << ui->cbChNameCountry;
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

  m_chapterModel->retranslateUi();
  // m_nameModel->retranslateUi();

  resizeChapterColumnsToContents();
}

void
Tab::resizeChapterColumnsToContents()
  const {
  Util::resizeViewColumnsToContents(ui->elements);
  // Util::resizeViewColumnsToContents(ui->tvChNames);
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

ChapterModel *
Tab::getChapterModel()
  const {
  return m_chapterModel;
}

void
Tab::newFile() {
}

void
Tab::resetData() {
  m_analyzer.reset();
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
Tab::copyControlsToStorage(QModelIndex const &) {
  // TODO: Tab::copyControlsToStorage
  mxinfo(boost::format("TODO: copyControlsFromStorage\n"));
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
  ui->cbChFlagEnabled->setChecked(!!FindChildValue<KaxChapterFlagEnabled>(*chapter));
  ui->cbChFlagHidden->setChecked(!!FindChildValue<KaxChapterFlagHidden>(*chapter));
  ui->leChUid->setText(QString::number(FindChildValue<KaxChapterUID>(*chapter)));
  ui->leChSegmentUid->setText(formatEbmlBinary(FindChild<KaxChapterSegmentUID>(*chapter)));
  ui->leChSegmentEditionUid->setText(segmentEditionUid ? QString::number(segmentEditionUid->GetValue()) : Q(""));

  // TODO: Tab::setChapterControlsFromStorage: names
  // m_namesModel->populate(GetChild<KaxChapterDisplay>(*chapter));
  for (auto const &widget : m_nameWidgets)
    widget->setEnabled(false);

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

void
Tab::chapterSelectionChanged(QItemSelection const &selected,
                             QItemSelection const &deselected) {
  if (!deselected.isEmpty()) {
    auto indexes = deselected.at(0).indexes();
    if (!indexes.isEmpty())
      copyControlsToStorage(indexes.at(0));
  }

  if (!selected.isEmpty()) {
    auto indexes = selected.at(0).indexes();
    if (!indexes.isEmpty() && setControlsFromStorage(indexes.at(0)))
      return;
  }

  ui->pageContainer->setCurrentWidget(ui->emptyPage);
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

}}}
