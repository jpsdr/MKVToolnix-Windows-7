#include "common/common_pch.h"

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

void
ChapterModel::setEditionRowText(QList<QStandardItem *> const &rowItems,
                                int row) {
  rowItems[0]->setText(QY("Edition entry %1").arg(row + 1));
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

  rowItems[0]->setText(chapterDisplayName(chapter));
  rowItems[1]->setText(kStart ? Q(format_timecode(kStart->GetValue(), 0)) : Q(""));
  rowItems[2]->setText(kEnd   ? Q(format_timecode(kEnd->GetValue(),   0)) : Q(""));
}

void
ChapterModel::appendEdition(EditionPtr const &edition) {
  auto rowItems = newRowItems();
  rowItems[0]->setData(QVariant::fromValue(edition), Util::ChapterEditorEditionRole);

  setEditionRowText(rowItems, rowCount());
  appendRow(rowItems);
}

void
ChapterModel::appendChapter(ChapterPtr const &chapter,
                            QModelIndex const &parentIdx) {
  auto rowItems = newRowItems();
  rowItems[0]->setData(QVariant::fromValue(chapter), Util::ChapterEditorChapterRole);

  setChapterRowText(rowItems);
  itemFromIndex(parentIdx)->appendRow(rowItems);
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

}}}
