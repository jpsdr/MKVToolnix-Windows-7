#ifndef MTX_MKVTOOLNIX_GUI_CHAPTER_EDITOR_CHAPTER_MODEL_H
#define MTX_MKVTOOLNIX_GUI_CHAPTER_EDITOR_CHAPTER_MODEL_H

#include "common/common_pch.h"

#include <QList>
#include <QStandardItemModel>

#include <matroska/KaxChapters.h>

// class QAbstractItemView;

using namespace libmatroska;

using EditionPtr = std::shared_ptr<KaxEditionEntry>;
using ChapterPtr  = std::shared_ptr<KaxChapterAtom>;

Q_DECLARE_METATYPE(EditionPtr);
Q_DECLARE_METATYPE(ChapterPtr);

namespace mtx { namespace gui { namespace ChapterEditor {

class ChapterModel: public QStandardItemModel {
  Q_OBJECT;
protected:
  // QList<ChapterBase *> m_chapters, m_topLevelChapters;

public:
  ChapterModel(QObject *parent);
  virtual ~ChapterModel();

  // ChapterBase *selectedChapter(QModelIndex const &idx) const;

  void appendEdition(EditionPtr const &edition);
  void appendChapter(ChapterPtr const &chapter, QModelIndex const &parentIdx);

  // QList<ChapterBase *> &getChapters();
  // QList<ChapterBase *> &getTopLevelChapters();

  void populate(EbmlMaster &master);
  void reset();
  void retranslateUi();

  // QModelIndex validate() const;

protected:
  void setEditionRowText(QList<QStandardItem *> const &rowItems, int row);
  void setChapterRowText(QList<QStandardItem *> const &rowItems);
  void populate(EbmlMaster &master, QModelIndex const &parentIdx);

protected:
  static QList<QStandardItem *> newRowItems();
  static QString chapterNameForLanguage(KaxChapterAtom &chapter, std::string const &language);
};

}}}

#endif  // MTX_MKVTOOLNIX_GUI_CHAPTER_EDITOR_CHAPTER_MODEL_H
