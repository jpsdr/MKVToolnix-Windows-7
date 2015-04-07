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
  connect(this, &QAbstractItemModel::rowsAboutToBeRemoved, this, &ChapterModel::invalidateRegistryEntriesBeforeRemoval);
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
  return std::static_pointer_cast<KaxChapterAtom>(m_elementRegistry[ registryIdFromItem(item) ]);
}

EditionPtr
ChapterModel::editionFromItem(QStandardItem *item) {
  return std::static_pointer_cast<KaxEditionEntry>(m_elementRegistry[ registryIdFromItem(item) ]);
}

void
ChapterModel::setChapterRowText(QList<QStandardItem *> const &rowItems) {
  auto chapter = chapterFromItem(rowItems[0]);
  if (!chapter)
    return;

  auto kStart = FindChild<KaxChapterTimeStart>(*chapter);
  auto kEnd   = FindChild<KaxChapterTimeEnd>(*chapter);

  rowItems[1]->setData(static_cast<qulonglong>(kStart ? kStart->GetValue() : 0), QStandardItemModel::sortRole());

  rowItems[0]->setText(chapterDisplayName(*chapter));
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
  rowItems[0]->setData(registerElement(std::static_pointer_cast<EbmlMaster>(edition)), Util::ChapterEditorChapterOrEditionRole);

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
  rowItems[0]->setData(registerElement(std::static_pointer_cast<EbmlMaster>(chapter)), Util::ChapterEditorChapterOrEditionRole);

  setChapterRowText(rowItems);
  itemFromIndex(parentIdx)->insertRow(row, rowItems);
}

void
ChapterModel::reset() {
  beginResetModel();

  m_elementRegistry.clear();
  m_nextElementRegistryIdx = 0;
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
ChapterModel::invalidateRegistryEntriesBeforeRemoval(QModelIndex const &parent,
                                                     int first,
                                                     int last) {
  for (auto row = first; row <= last; ++row)
    walkTree(index(row, 0, parent), [this](QModelIndex const &idx) {
      auto id = registryIdFromItem(itemFromIndex(idx));
      mxinfo(boost::format("invalidating id %1%\n") % id);
      m_elementRegistry.remove(id);
    });
}

void
ChapterModel::populate(EbmlMaster &master) {
  beginResetModel();

  m_elementRegistry.clear();
  m_nextElementRegistryIdx = 0;

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

QModelIndex
ChapterModel::duplicateTree(QModelIndex const &srcIdx) {
  if (!srcIdx.isValid())
    return {};

  duplicateTree(srcIdx.parent(), srcIdx.row() + 1, srcIdx);

  return index(srcIdx.row() + 1, 0, srcIdx.parent());
}

void
ChapterModel::duplicateTree(QModelIndex const &destParentIdx,
                            int destRow,
                            QModelIndex const &srcIdx) {
  auto srcItem = itemFromIndex(srcIdx);

  if (destParentIdx.isValid()) {
    auto newChapter = clone(chapterFromItem(srcItem));
    insertChapter(destRow, newChapter, destParentIdx);

  } else {
    auto newEdition = clone(editionFromItem(srcItem));
    insertEdition(destRow, newEdition);
  }

  auto newDestParentIdx = index(destRow, 0, destParentIdx);

  for (auto row = 0, numRows = rowCount(srcIdx); row < numRows; ++row)
    duplicateTree(newDestParentIdx, row, index(row, 0, srcIdx));
}

void
ChapterModel::cloneElementsForRetrieval(QModelIndex const &parentIdx,
                                        EbmlMaster &target) {
  for (auto row = 0, numRows = rowCount(parentIdx); row < numRows; ++row) {
    auto elementIdx  = index(row, 0, parentIdx);
    auto elementItem = itemFromIndex(elementIdx);
    auto newElement  = static_cast<EbmlMaster *>(parentIdx.isValid() ? chapterFromItem(elementItem)->Clone() : editionFromItem(elementItem)->Clone());

    target.PushElement(*newElement);

    cloneElementsForRetrieval(elementIdx, *newElement);
  }
}

ChaptersPtr
ChapterModel::allChapters() {
  auto chapters = std::make_shared<KaxChapters>();
  cloneElementsForRetrieval(QModelIndex{}, *chapters);

  return chapters;
}

void
ChapterModel::fixMandatoryElements(QModelIndex const &parentIdx) {
  walkTree(parentIdx, [this](QModelIndex const &idx) {
    auto element = m_elementRegistry[ registryIdFromItem(itemFromIndex(idx)) ];
    if (element)
      fix_mandatory_chapter_elements(element.get());
  });
}

void
ChapterModel::walkTree(QModelIndex const &idx,
                       std::function<void(QModelIndex const &)> const &worker) {
  worker(idx);

  for (auto row = 0, numRows = rowCount(idx); row < numRows; ++row)
    walkTree(index(row, 0, idx), worker);
}

qulonglong
ChapterModel::registerElement(std::shared_ptr<EbmlMaster> const &element) {
  m_elementRegistry[ ++m_nextElementRegistryIdx ] = element;
  return m_nextElementRegistryIdx;
}

qulonglong
ChapterModel::registryIdFromItem(QStandardItem *item) {
  return item ? item->data(Util::ChapterEditorChapterOrEditionRole).value<qulonglong>() : 0;
}

}}}
