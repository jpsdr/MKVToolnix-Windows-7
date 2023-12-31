#pragma once

#include "common/common_pch.h"

#include <matroska/KaxSemantic.h>

#include <QIcon>
#include <QList>
#include <QStandardItemModel>

// class QAbstractItemView;

using ChaptersPtr = std::shared_ptr<libmatroska::KaxChapters>;
using EditionPtr  = std::shared_ptr<libmatroska::KaxEditionEntry>;
using ChapterPtr  = std::shared_ptr<libmatroska::KaxChapterAtom>;

Q_DECLARE_METATYPE(EditionPtr)
Q_DECLARE_METATYPE(ChapterPtr)

namespace mtx::gui::ChapterEditor {

class ChapterModel: public QStandardItemModel {
  Q_OBJECT

protected:
  QHash<qulonglong, std::shared_ptr<libebml::EbmlMaster>> m_elementRegistry;
  qulonglong m_nextElementRegistryIdx{};

  QModelIndex m_selectedIdx;

public:
  ChapterModel(QObject *parent);
  virtual ~ChapterModel();

  void appendEdition(EditionPtr const &edition);
  void insertEdition(int row, EditionPtr const &edition);
  void appendChapter(ChapterPtr const &chapter, QModelIndex const &parentIdx);
  void insertChapter(int row, ChapterPtr const &chapter, QModelIndex const &parentIdx);

  QModelIndex duplicateTree(QModelIndex const &srcIdx);
  void removeTree(QModelIndex const &idx);

  void updateRow(QModelIndex const &idx);
  void populate(libebml::EbmlMaster &master, bool append);
  void reset();
  void retranslateUi();

  void fixMandatoryElements(QModelIndex const &parentIdx = QModelIndex{});

  ChapterPtr chapterFromItem(QStandardItem *item);
  EditionPtr editionFromItem(QStandardItem *item);

  ChaptersPtr allChapters();

  void setSelectedIdx(QModelIndex const &idx);

  virtual Qt::DropActions supportedDropActions() const override;
  virtual Qt::ItemFlags flags(QModelIndex const &idx) const override;
  virtual bool canDropMimeData(QMimeData const *data, Qt::DropAction action, int row, int column, QModelIndex const &parent) const override;
  virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

  ChaptersPtr cloneSubtreeForRetrieval(QModelIndex const &topIdx);

protected:
  void setEditionRowText(QList<QStandardItem *> const &rowItems);
  void setChapterRowText(QList<QStandardItem *> const &rowItems);
  void populate(libebml::EbmlMaster &master, QModelIndex const &parentIdx);
  QList<QStandardItem *> itemsForRow(QModelIndex const &idx);
  void cloneElementsForRetrieval(QModelIndex const &parentIdx, libebml::EbmlMaster &target);
  void duplicateTree(QModelIndex const &destParentIdx, int destRow, QModelIndex const &srcIdx);

  void collectUsedEditionAndChapterUIDs(QModelIndex const &parentIdx, QSet<uint64_t> &usedEditionUIDs, QSet<uint64_t> &usedChapterUIDs);
  void fixEditionAndChapterUIDs(libebml::EbmlMaster &master);
  void fixEditionAndChapterUIDs(libebml::EbmlMaster &master, QSet<uint64_t> &usedEditionUIDs, QSet<uint64_t> &usedChapterUIDs);

  qulonglong registerElement(std::shared_ptr<libebml::EbmlMaster> const &element);
  qulonglong registryIdFromItem(QStandardItem *item);

protected:
  static QList<QStandardItem *> newRowItems();

public:
  static QString chapterDisplayName(libmatroska::KaxChapterAtom &chapter);
  static QString chapterNameForLanguage(libmatroska::KaxChapterAtom &chapter, std::string const &language);
};

}
