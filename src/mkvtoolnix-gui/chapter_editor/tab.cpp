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
#include "mkvtoolnix-gui/forms/chapter_editor/tab.h"
#include "mkvtoolnix-gui/chapter_editor/chapter_model.h"
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

  // connect(ui->elements->selectionModel(), &QItemSelectionModel::currentChanged, this, &Tab::selectionChanged);
  connect(m_expandAllAction,              &QAction::triggered,                  this, &Tab::expandAll);
  connect(m_collapseAllAction,            &QAction::triggered,                  this, &Tab::collapseAll);
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

  resizeChapterColumnsToContents();
}

void
Tab::resizeChapterColumnsToContents()
  const {
  Util::resizeViewColumnsToContents(ui->elements);
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

// void
// Tab::appendPage(PageBase *page,
//                 QModelIndex const &parentIdx) {
//   ui->pageContainer->addWidget(page);
//   m_chapterModel->appendPage(page, parentIdx);
// }

// void
// Tab::selectionChanged(QModelIndex const &current,
//                       QModelIndex const &) {
//   auto selectedPage = m_chapterModel->selectedPage(current);
//   if (selectedPage)
//     ui->pageContainer->setCurrentWidget(selectedPage);
// }

// bool
// Tab::hasBeenModified() {
//   auto pages = m_chapterModel->getTopLevelPages();
//   for (auto const &page : pages)
//     if (page->hasBeenModified())
//       return true;

//   return false;
// }

// void
// Tab::validate() {
//   auto pageIdx = m_chapterModel->validate();

//   if (!pageIdx.isValid()) {
//     QMessageBox::information(this, QY("Chapter validation"), QY("All chapter values are OK."));
//     return;
//   }

//   reportValidationFailure(false, pageIdx);
// }

// void
// Tab::reportValidationFailure(bool isCritical,
//                              QModelIndex const &pageIdx) {
//   ui->elements->selectionModel()->select(pageIdx, QItemSelectionModel::ClearAndSelect);
//   selectionChanged(pageIdx, QModelIndex{});

//   if (isCritical)
//     QMessageBox::critical(this, QY("Chapter validation"), QY("There were errors in the chapter values preventing the chapters from being saved. The first error has been selected."));
//   else
//     QMessageBox::warning(this, QY("Chapter validation"), QY("There were errors in the chapter values preventing the chapters from being saved. The first error has been selected."));
// }

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

}}}
