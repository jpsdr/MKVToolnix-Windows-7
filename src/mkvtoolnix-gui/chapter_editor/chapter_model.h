#ifndef MTX_MKVTOOLNIX_GUI_CHAPTER_EDITOR_CHAPTER_MODEL_H
#define MTX_MKVTOOLNIX_GUI_CHAPTER_EDITOR_CHAPTER_MODEL_H

#include "common/common_pch.h"

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

public:
  ChapterModel(QObject *parent);
  virtual ~ChapterModel();

  void appendEdition(EditionPtr const &edition);
  void insertEdition(int row, EditionPtr const &edition);
  void appendChapter(ChapterPtr const &chapter, QModelIndex const &parentIdx);
  void insertChapter(int row, ChapterPtr const &chapter, QModelIndex const &parentIdx);

  void updateRow(QModelIndex const &idx);
  void populate(EbmlMaster &master);
  void reset();
  void retranslateUi();

  void fixMandatoryElements(QModelIndex const &parentIdx = QModelIndex{});

  ChapterPtr chapterFromItem(QStandardItem *item);
  EditionPtr editionFromItem(QStandardItem *item);

  ChaptersPtr allChapters();

protected:
  void setEditionRowText(QList<QStandardItem *> const &rowItems);
  void setChapterRowText(QList<QStandardItem *> const &rowItems);
  void populate(EbmlMaster &master, QModelIndex const &parentIdx);
  QList<QStandardItem *> itemsForRow(QModelIndex const &idx);
  void cloneElements(QModelIndex const &parentIdx, EbmlMaster &target);


protected:
  static QList<QStandardItem *> newRowItems();

public:
  static QString chapterDisplayName(KaxChapterAtom &chapter);
  static QString chapterNameForLanguage(KaxChapterAtom &chapter, std::string const &language);
};

}}}

#endif  // MTX_MKVTOOLNIX_GUI_CHAPTER_EDITOR_CHAPTER_MODEL_H
