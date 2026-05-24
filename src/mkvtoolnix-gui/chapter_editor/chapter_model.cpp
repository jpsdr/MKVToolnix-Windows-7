#include "common/common_pch.h"

#include <QDebug>

#include "common/chapters/chapters.h"
#include "common/ebml.h"
#include "common/logger.h"
#include "common/qt.h"
#include "common/strings/formatting.h"
#include "common/unique_numbers.h"
#include "mkvtoolnix-gui/chapter_editor/chapter_model.h"
#include "mkvtoolnix-gui/chapter_editor/name_model.h"
#include "mkvtoolnix-gui/util/model.h"

using namespace mtx::gui;

namespace mtx::gui::ChapterEditor {

static auto const s_numColumns = 4;

ChapterModel::ChapterModel(QObject *parent)
  : QStandardItemModel{parent}
{
}

ChapterModel::~ChapterModel() {
}

void
ChapterModel::retranslateUi() {
  Util::setDisplayableAndSymbolicColumnNames(*this, {
    { QY("Edition/Chapter"), Q("editionChapter") },
    { QY("Start"),           Q("start")          },
    { QY("End"),             Q("end")            },
    { QY("Flags"),           Q("flags")          },
  });

  Util::walkTree(*this, QModelIndex{}, [this](QModelIndex const &currentIdx) {
    updateRow(currentIdx);
  });
}

QList<QStandardItem *>
ChapterModel::newRowItems() {
  auto rowItems = QList<QStandardItem *>{};

  for (auto column = 0; column < s_numColumns; ++column)
    rowItems << new QStandardItem{};

  return rowItems;
}

QList<QStandardItem *>
ChapterModel::itemsForRow(QModelIndex const &idx) {
  auto rowItems = QList<QStandardItem *>{};

  for (auto column = 0; column < s_numColumns; ++column)
    rowItems << itemFromIndex(idx.sibling(idx.row(), column));

  return rowItems;
}

void
ChapterModel::setEditionRowText(QList<QStandardItem *> const &rowItems) {
  auto edition = editionFromItem(rowItems[0]);
  if (!edition)
    return;

  auto flags     = QStringList{};
  auto isDefault = edition && find_child_value<libmatroska::KaxEditionFlagDefault>(*edition);
  auto isHidden  = edition && find_child_value<libmatroska::KaxEditionFlagHidden>(*edition);
  auto isOrdered = edition && find_child_value<libmatroska::KaxEditionFlagOrdered>(*edition);

  if (isOrdered)
    flags << QY("Ordered");
  if (isHidden)
    flags << QY("Hidden");
  if (isDefault)
    flags << QY("Default");

  rowItems[0]->setText(QY("Edition entry"));
  rowItems[3]->setText(flags.join(Q(", ")));
}

ChapterPtr
ChapterModel::chapterFromItem(QStandardItem *item) {
  return std::static_pointer_cast<libmatroska::KaxChapterAtom>(m_elementRegistry[ registryIdFromItem(item) ]);
}

EditionPtr
ChapterModel::editionFromItem(QStandardItem *item) {
  return std::static_pointer_cast<libmatroska::KaxEditionEntry>(m_elementRegistry[ registryIdFromItem(item) ]);
}

void
ChapterModel::setChapterRowText(QList<QStandardItem *> const &rowItems) {
  auto chapter = chapterFromItem(rowItems[0]);
  if (!chapter)
    return;

  auto flags     = QStringList{};

  auto isEnabled = find_child_value<libmatroska::KaxChapterFlagEnabled>(*chapter, 1);
  auto isHidden  = find_child_value<libmatroska::KaxChapterFlagHidden>(*chapter);

  auto kStart    = find_child<libmatroska::KaxChapterTimeStart>(*chapter);
  auto kEnd      = find_child<libmatroska::KaxChapterTimeEnd>(*chapter);

  if (!isEnabled)
    flags << QY("Disabled");
  if (isHidden)
    flags << QY("Hidden");

  rowItems[1]->setData(static_cast<qulonglong>(kStart ? kStart->GetValue() : 0), QStandardItemModel::sortRole());

  rowItems[0]->setText(chapterDisplayName(*chapter));
  rowItems[1]->setText(kStart ? Q(mtx::string::format_timestamp(kStart->GetValue(), 9)) : Q(""));
  rowItems[2]->setText(kEnd   ? Q(mtx::string::format_timestamp(kEnd->GetValue(),   9)) : Q(""));
  rowItems[3]->setText(flags.join(Q(", ")));
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
  rowItems[0]->setData(registerElement(std::static_pointer_cast<libebml::EbmlMaster>(edition)), Util::ChapterEditorChapterOrEditionRole);

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
  rowItems[0]->setData(registerElement(std::static_pointer_cast<libebml::EbmlMaster>(chapter)), Util::ChapterEditorChapterOrEditionRole);

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
ChapterModel::chapterNameForLanguage(libmatroska::KaxChapterAtom &chapter,
                                     std::string const &language) {
  for (auto const &child : chapter) {
    auto kDisplay = dynamic_cast<libmatroska::KaxChapterDisplay *>(child);
    if (!kDisplay)
      continue;

    auto lists          = NameModel::effectiveLanguagesForDisplay(*kDisplay);
    auto actualLanguage = mtx::chapters::get_language_from_display(*kDisplay, "eng"s);

    if (   language.empty()
        || (std::find_if(lists.languageCodes.begin(), lists.languageCodes.end(), [&language](auto const &actualLanguage) {
              return (language == actualLanguage.get_language())
                  || (language == actualLanguage.get_iso639_alpha_3_code());
            }) != lists.languageCodes.end()))
      return Q(find_child_value<libmatroska::KaxChapterString>(kDisplay));
  }

  return Q("");
}

QString
ChapterModel::chapterDisplayName(libmatroska::KaxChapterAtom &chapter) {
  auto chapterName = chapterNameForLanguage(chapter, "und");

  if (chapterName.isEmpty())
    chapterName = chapterNameForLanguage(chapter, "");

  if (chapterName.isEmpty())
    chapterName = QY("<Unnamed>");

  return chapterName;
}

void
ChapterModel::removeTree(QModelIndex const &idx) {
  if (!idx.isValid())
    return;

  Util::walkTree(*this, idx, [this](QModelIndex const &currIdx) {
    m_elementRegistry.remove(registryIdFromItem(itemFromIndex(currIdx)));
  });

  removeRow(idx.row(), idx.parent());
}

void
ChapterModel::populate(libebml::EbmlMaster &master,
                       bool append) {
  beginResetModel();

  if (!append) {
    m_elementRegistry.clear();
    m_nextElementRegistryIdx = 0;

    removeRows(0, rowCount());

  } else
    fixEditionAndChapterUIDs(master);

  populate(master, QModelIndex{});

  endResetModel();
}

void
ChapterModel::populate(libebml::EbmlMaster &master,
                       QModelIndex const &parentIdx) {
  auto masterIdx = 0u;
  while (masterIdx < master.ListSize()) {
    auto element = master[masterIdx];
    auto edition = dynamic_cast<libmatroska::KaxEditionEntry *>(element);
    if (edition) {
      if (!parentIdx.isValid()) {
        appendEdition(EditionPtr{edition});
        populate(*edition, index(rowCount(parentIdx) - 1, 0, parentIdx));
      }
      master.Remove(masterIdx);
      continue;
    }

    auto chapter = dynamic_cast<libmatroska::KaxChapterAtom *>(element);
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
                                        libebml::EbmlMaster &target) {
  for (auto row = 0, numRows = rowCount(parentIdx); row < numRows; ++row) {
    auto elementIdx  = index(row, 0, parentIdx);
    auto elementItem = itemFromIndex(elementIdx);
    auto newElement  = static_cast<libebml::EbmlMaster *>(parentIdx.isValid() ? chapterFromItem(elementItem)->Clone() : editionFromItem(elementItem)->Clone());

    target.PushElement(*newElement);

    cloneElementsForRetrieval(elementIdx, *newElement);
  }
}

ChaptersPtr
ChapterModel::cloneSubtreeForRetrieval(QModelIndex const &topIdx) {
  auto newChapters  = std::make_shared<libmatroska::KaxChapters>();
  auto topItem      = itemFromIndex(topIdx);
  auto topIsEdition = !topIdx.parent().isValid();
  auto newElement   = static_cast<libebml::EbmlMaster *>(topIsEdition ? editionFromItem(topItem)->Clone() : chapterFromItem(topItem)->Clone());

  if (topIsEdition)
    newChapters->PushElement(*newElement);

  else {
    newChapters->PushElement(*new libmatroska::KaxEditionEntry);
    static_cast<libmatroska::KaxEditionEntry *>((*newChapters)[0])->PushElement(*newElement);
  }

  cloneElementsForRetrieval(topIdx, *newElement);

  return newChapters;
}

void
ChapterModel::collectUsedEditionAndChapterUIDs(QModelIndex const &parentIdx,
                                               QSet<uint64_t> &usedEditionUIDs,
                                               QSet<uint64_t> &usedChapterUIDs) {
  for (auto row = 0, numRows = rowCount(parentIdx); row < numRows; ++row) {
    auto elementIdx  = index(row, 0, parentIdx);
    auto elementItem = itemFromIndex(elementIdx);
    auto uid         = !parentIdx.isValid() ? find_child_value<libmatroska::KaxEditionUID>(*editionFromItem(elementItem)) : find_child_value<libmatroska::KaxChapterUID>(*chapterFromItem(elementItem));

    if (uid) {
      if (!parentIdx.isValid())
        usedEditionUIDs << uid;
      else
        usedChapterUIDs << uid;
    }

    collectUsedEditionAndChapterUIDs(elementIdx, usedEditionUIDs, usedChapterUIDs);
  }
}

void
ChapterModel::fixEditionAndChapterUIDs(libebml::EbmlMaster &master,
                                       QSet<uint64_t> &usedEditionUIDs,
                                       QSet<uint64_t> &usedChapterUIDs) {
  auto isEdition  = dynamic_cast<libmatroska::KaxEditionEntry *>(&master);
  auto uidElement = isEdition ? static_cast<libebml::EbmlUInteger *>(find_child<libmatroska::KaxEditionUID>(master)) : static_cast<libebml::EbmlUInteger *>(find_child<libmatroska::KaxChapterUID>(master));

  if (uidElement && uidElement->GetValue()) {
    auto &set = isEdition ? usedEditionUIDs : usedChapterUIDs;

    if (set.contains(uidElement->GetValue())) {
      uint64_t newValue;
      do {
        newValue = create_unique_number(isEdition ? UNIQUE_EDITION_IDS : UNIQUE_CHAPTER_IDS);
      } while (set.contains(newValue));

      uidElement->SetValue(newValue);
    }

    set << uidElement->GetValue();
  }

  for (auto & child : master)
    if (dynamic_cast<libebml::EbmlMaster *>(child))
      fixEditionAndChapterUIDs(*dynamic_cast<libebml::EbmlMaster *>(child), usedEditionUIDs, usedChapterUIDs);
}

void
ChapterModel::fixEditionAndChapterUIDs(libebml::EbmlMaster &master) {
  QSet<uint64_t> usedEditionUIDs, usedChapterUIDs;

  collectUsedEditionAndChapterUIDs({}, usedEditionUIDs, usedChapterUIDs);
  fixEditionAndChapterUIDs(master, usedEditionUIDs, usedChapterUIDs);
}

ChaptersPtr
ChapterModel::allChapters() {
  auto chapters = std::make_shared<libmatroska::KaxChapters>();
  cloneElementsForRetrieval(QModelIndex{}, *chapters);

  mtx::chapters::unify_legacy_and_bcp47_languages_and_countries(*chapters);

  return chapters;
}

void
ChapterModel::fixMandatoryElements(QModelIndex const &parentIdx) {
  Util::walkTree(*this, parentIdx, [this](QModelIndex const &idx) {
    auto element = m_elementRegistry[ registryIdFromItem(itemFromIndex(idx)) ];
    if (!element)
      return;

    if (is_type<libmatroska::KaxChapterAtom>(*element) && !find_child_value<libmatroska::KaxChapterUID>(*element, 0))
      delete_children<libmatroska::KaxChapterUID>(*element);

    else if (is_type<libmatroska::KaxEditionEntry>(*element) && !find_child_value<libmatroska::KaxEditionUID>(*element, 0))
      delete_children<libmatroska::KaxEditionUID>(*element);

    fix_mandatory_elements(element.get());
  });
}

qulonglong
ChapterModel::registerElement(std::shared_ptr<libebml::EbmlMaster> const &element) {
  m_elementRegistry[ ++m_nextElementRegistryIdx ] = element;
  return m_nextElementRegistryIdx;
}

qulonglong
ChapterModel::registryIdFromItem(QStandardItem *item) {
  return item ? item->data(Util::ChapterEditorChapterOrEditionRole).value<qulonglong>() : 0;
}

void
ChapterModel::setSelectedIdx(QModelIndex const &idx) {
  m_selectedIdx = idx;
}

Qt::DropActions
ChapterModel::supportedDropActions()
  const {
  return Qt::MoveAction;
}

bool
ChapterModel::canDropMimeData(QMimeData const *,
                              Qt::DropAction action,
                              int row,
                              int column,
                              QModelIndex const &parent)
  const {
  auto selectedIsChapter = m_selectedIdx.isValid() && m_selectedIdx.parent().isValid();

  auto ok = (Qt::MoveAction == action)
         && (   ( selectedIsChapter &&  parent.isValid())
             || (!selectedIsChapter && !parent.isValid()));

  qDebug() << "mtx chapters canDropMimeData ok" << ok << "selectedIsChapter" << selectedIsChapter << "row" << row << "column" << column << "parent" << parent;

  return ok;
}

bool
ChapterModel::dropMimeData(QMimeData const *data,
                           Qt::DropAction action,
                           int row,
                           int column,
                           QModelIndex const &parent) {
  if (!canDropMimeData(data, action, row, column, parent))
    return false;

  auto isInside = (-1 == row) && (-1 == column);
  auto result   = QStandardItemModel::dropMimeData(data, action, isInside ? -1 : row, isInside ? -1 : 0, parent.sibling(parent.row(), 0));

  Util::requestAllItems(*this);

  return result;
}

Qt::ItemFlags
ChapterModel::flags(QModelIndex const &idx)
  const {
  auto selectedIsChapter = m_selectedIdx.isValid() && m_selectedIdx.parent().isValid();

  auto effectiveFlags = QFlags<Qt::ItemFlag>{};

  // Querying the canvas? Cannot be dragged, and only editions may be
  // dropped on it.
  if (!idx.isValid())
    effectiveFlags = selectedIsChapter ? Qt::NoItemFlags : Qt::ItemIsDropEnabled;

  else {
    // Querying an edition? An edition may always be dragged, but only
    // chapters may be dropped on it. The same holds true for chapters,
    // though.
    auto defaultFlags = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled;
    effectiveFlags    = selectedIsChapter ? defaultFlags | Qt::ItemIsDropEnabled : defaultFlags;
  }

  // log_it(fmt::format("sele is {0}/{1} ({2}/{3}) seleIsChap {4} flags {5:04x} {6}\n",
  //        m_selectedIdx.row(), m_selectedIdx.column(), m_selectedIdx.parent().row(), m_selectedIdx.parent().column(), selectedIsChapter,
  //        effectiveFlags, Util::itemFlagsToString(effectiveFlags)));

  return effectiveFlags;
}

}
