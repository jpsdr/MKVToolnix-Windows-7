#ifndef MTX_MKVTOOLNIX_GUI_CHAPTER_EDITOR_NAME_MODEL_H
#define MTX_MKVTOOLNIX_GUI_CHAPTER_EDITOR_NAME_MODEL_H

#include "common/common_pch.h"

#include <QList>
#include <QStandardItemModel>

#include <matroska/KaxChapters.h>

// class QAbstractItemView;

Q_DECLARE_METATYPE(libmatroska::KaxChapterDisplay *);

using namespace libmatroska;

namespace mtx { namespace gui { namespace ChapterEditor {

class NameModel: public QStandardItemModel {
  Q_OBJECT;

protected:
  KaxChapterAtom *m_chapter{};
  QHash<qulonglong, KaxChapterDisplay *> m_displayRegistry;
  qulonglong m_nextDisplayRegistryIdx{};

public:
  NameModel(QObject *parent);
  virtual ~NameModel();

  void append(KaxChapterDisplay &display);
  void addNew();
  void remove(QModelIndex const &idx);
  void updateRow(int row);

  void populate(KaxChapterAtom &chapter);
  void reset();
  void retranslateUi();

  KaxChapterDisplay *displayFromIndex(QModelIndex const &idx);
  KaxChapterDisplay *displayFromItem(QStandardItem *item);

  virtual Qt::DropActions supportedDropActions() const override;
  virtual Qt::ItemFlags flags(QModelIndex const &index) const override;


protected:
  void setRowText(QList<QStandardItem *> const &rowItems);
  QList<QStandardItem *> itemsForRow(int row);
  qulonglong registerDisplay(KaxChapterDisplay &display);
  qulonglong registryIdFromItem(QStandardItem *item);

protected:
  static QList<QStandardItem *> newRowItems();
};

}}}

#endif  // MTX_MKVTOOLNIX_GUI_CHAPTER_EDITOR_NAME_MODEL_H
