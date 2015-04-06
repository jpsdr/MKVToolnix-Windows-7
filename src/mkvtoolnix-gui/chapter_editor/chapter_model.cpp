#include "common/common_pch.h"

#include "common/chapters/chapters.h"
#include "common/ebml.h"
#include "common/qt.h"
#include "common/strings/formatting.h"
#include "mkvtoolnix-gui/chapter_editor/chapter_model.h"
#include "mkvtoolnix-gui/util/util.h"

namespace mtx { namespace gui { namespace ChapterEditor {

using namespace mtx::gui;

ChapterModel::ChapterModel(QObject *parent)
  : QStandardItemModel{parent}
{
}

ChapterModel::~ChapterModel() {
}

void
ChapterModel::retranslateUi() {
  auto labels = QStringList{} << QY("Edition/Chapter") << QY("Start") << QY("End");
  setHorizontalHeaderLabels(labels);
}

QList<QStandardItem *>
ChapterModel::newRowItems() {
  auto items = QList<QStandardItem *>{} << new QStandardItem{} << new QStandardItem{} << new QStandardItem{};
  return items;
}

QList<QStandardItem *>
ChapterModel::itemsForRow(QModelIndex const &idx) {
  auto rowItems = QList<QStandardItem *>{};

  for (auto column = 0; 3 > column; ++column)
    rowItems << itemFromIndex(idx.sibling(idx.row(), column));

  return rowItems;
}

void
ChapterModel::setEditionRowText(QList<QStandardItem *> const &rowItems) {
  rowItems[0]->setText(QY("Edition entry"));
}

ChapterPtr
ChapterModel::chapterFromItem(QStandardItem *item) {
  return item ? item->data(Util::ChapterEditorChapterRole).value<ChapterPtr>() : ChapterPtr{};
}

EditionPtr
ChapterModel::editionFromItem(QStandardItem *item) {
  return item ? item->data(Util::ChapterEditorEditionRole).value<EditionPtr>() : EditionPtr{};
}

void
ChapterModel::setChapterRowText(QList<QStandardItem *> const &rowItems) {
  auto &chapter = *chapterFromItem(rowItems[0]);

  auto kStart   = FindChild<KaxChapterTimeStart>(chapter);
  auto kEnd     = FindChild<KaxChapterTimeEnd>(chapter);

  rowItems[1]->setData(static_cast<qulonglong>(kStart ? kStart->GetValue() : 0), QStandardItemModel::sortRole());

  rowItems[0]->setText(chapterDisplayName(chapter));
  rowItems[1]->setText(kStart ? Q(format_timecode(kStart->GetValue(), 0)) : Q(""));
  rowItems[2]->setText(kEnd   ? Q(format_timecode(kEnd->GetValue(),   0)) : Q(""));
}

void
ChapterModel::updateRow(QModelIndex const &idx) {
  if (!idx.isValid())
    return;

  if (idx.parent().isValid())
    setChapterRowText(itemsForRow(idx));
  else
    setEditionRowText(itemsForRow(idx));
}

void
ChapterModel::appendEdition(EditionPtr const &edition) {
  insertEdition(rowCount(), edition);
}

void
ChapterModel::insertEdition(int row,
                            EditionPtr const &edition) {
  auto rowItems = newRowItems();
  rowItems[0]->setData(QVariant::fromValue(edition), Util::ChapterEditorEditionRole);

  setEditionRowText(rowItems);
  insertRow(row, rowItems);
}

void
ChapterModel::appendChapter(ChapterPtr const &chapter,
                            QModelIndex const &parentIdx) {
  insertChapter(rowCount(parentIdx), chapter, parentIdx);
}

void
ChapterModel::insertChapter(int row,
                            ChapterPtr const &chapter,
                            QModelIndex const &parentIdx) {
  auto rowItems = newRowItems();
  rowItems[0]->setData(QVariant::fromValue(chapter), Util::ChapterEditorChapterRole);

  setChapterRowText(rowItems);
  itemFromIndex(parentIdx)->insertRow(row, rowItems);
}

void
ChapterModel::reset() {
  beginResetModel();
  removeRows(0, rowCount());
  endResetModel();
}

QString
ChapterModel::chapterNameForLanguage(KaxChapterAtom &chapter,
                                     std::string const &language) {
  for (auto const &element : chapter) {
    auto kDisplay = dynamic_cast<KaxChapterDisplay *>(element);
    if (!kDisplay)
      continue;

    auto actualLanguage = FindChildValue<KaxChapterLanguage>(kDisplay, std::string{"eng"});
    if (language.empty() || (language == actualLanguage))
      return Q(FindChildValue<KaxChapterString>(kDisplay));
  }

  return Q("");
}

QString
ChapterModel::chapterDisplayName(KaxChapterAtom &chapter) {
  auto chapterName = chapterNameForLanguage(chapter, "und");

  if (chapterName.isEmpty())
    chapterName = chapterNameForLanguage(chapter, "");

  if (chapterName.isEmpty())
    chapterName = QY("<unnamed>");

  return chapterName;
}

void
ChapterModel::populate(EbmlMaster &master) {
  beginResetModel();
  removeRows(0, rowCount());
  populate(master, QModelIndex{});
  endResetModel();
}

void
ChapterModel::populate(EbmlMaster &master,
                       QModelIndex const &parentIdx) {
  auto masterIdx = 0u;
  while (masterIdx < master.ListSize()) {
    auto element = master[masterIdx];
    auto edition = dynamic_cast<KaxEditionEntry *>(element);
    if (edition) {
      if (!parentIdx.isValid()) {
        appendEdition(EditionPtr{edition});
        populate(*edition, index(rowCount(parentIdx) - 1, 0, parentIdx));
      }
      master.Remove(masterIdx);
      continue;
    }

    auto chapter = dynamic_cast<KaxChapterAtom *>(element);
    if (chapter) {
      if (parentIdx.isValid()) {
        appendChapter(ChapterPtr{chapter}, parentIdx);
        populate(*chapter, index(rowCount(parentIdx) - 1, 0, parentIdx));
      }
      master.Remove(masterIdx);
      continue;
    }

    ++masterIdx;
  }
}

void
ChapterModel::cloneElements(QModelIndex const &parentIdx,
                            EbmlMaster &target) {
  for (auto row = 0, numRows = rowCount(parentIdx); row < numRows; ++row) {
    auto elementIdx  = index(row, 0, parentIdx);
    auto elementItem = itemFromIndex(elementIdx);
    auto newElement  = static_cast<EbmlMaster *>(parentIdx.isValid() ? chapterFromItem(elementItem)->Clone() : editionFromItem(elementItem)->Clone());

    target.PushElement(*newElement);

    cloneElements(elementIdx, *newElement);
  }
}

ChaptersPtr
ChapterModel::allChapters() {
  auto chapters = std::make_shared<KaxChapters>();
  cloneElements(QModelIndex{}, *chapters);

  return chapters;
}

void
ChapterModel::fixMandatoryElements(QModelIndex const &parentIdx) {
  for (auto row = 0, numRows = rowCount(parentIdx); row < numRows; ++row) {
    auto elementIdx  = index(row, 0, parentIdx);
    auto elementItem = itemFromIndex(elementIdx);
    auto element     = parentIdx.isValid() ? static_cast<EbmlMaster *>(chapterFromItem(elementItem).get()) : static_cast<EbmlMaster *>(editionFromItem(elementItem).get());

    fix_mandatory_chapter_elements(element);

    fixMandatoryElements(elementIdx);
  }
}

}}}
