#pragma once

#include "common/common_pch.h"

#include <QIcon>
#include <QList>
#include <QStandardItemModel>

#include <matroska/KaxChapters.h>

// class QAbstractItemView;

using namespace libmatroska;

using ChaptersPtr = std::shared_ptr<KaxChapters>;
using EditionPtr  = std::shared_ptr<KaxEditionEntry>;
using ChapterPtr  = std::shared_ptr<KaxChapterAtom>;

Q_DECLARE_METATYPE(EditionPtr);
Q_DECLARE_METATYPE(ChapterPtr);

namespace mtx { namespace gui { namespace ChapterEditor {

class ChapterModel: public QStandardItemModel {
  Q_OBJECT;

protected:
  QHash<qulonglong, std::shared_ptr<EbmlMaster>> m_elementRegistry;
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
  void populate(EbmlMaster &master);
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

protected:
  void setEditionRowText(QList<QStandardItem *> const &rowItems);
  void setChapterRowText(QList<QStandardItem *> const &rowItems);
  void populate(EbmlMaster &master, QModelIndex const &parentIdx);
  QList<QStandardItem *> itemsForRow(QModelIndex const &idx);
  void cloneElementsForRetrieval(QModelIndex const &parentIdx, EbmlMaster &target);
  void duplicateTree(QModelIndex const &destParentIdx, int destRow, QModelIndex const &srcIdx);

  qulonglong registerElement(std::shared_ptr<EbmlMaster> const &element);
  qulonglong registryIdFromItem(QStandardItem *item);

protected:
  static QList<QStandardItem *> newRowItems();

public:
  static QString chapterDisplayName(KaxChapterAtom &chapter);
  static QString chapterNameForLanguage(KaxChapterAtom &chapter, std::string const &language);
};

}}}
