#pragma once

#include "common/common_pch.h"

#include <QList>
#include <QStandardItemModel>

#include <matroska/KaxChapters.h>

// class QAbstractItemView;

Q_DECLARE_METATYPE(libmatroska::KaxChapterDisplay *)

namespace mtx { namespace gui { namespace ChapterEditor {

class NameModel: public QStandardItemModel {
  Q_OBJECT

protected:
  libmatroska::KaxChapterAtom *m_chapter{};
  QHash<qulonglong, libmatroska::KaxChapterDisplay *> m_displayRegistry;
  qulonglong m_nextDisplayRegistryIdx{};

public:
  NameModel(QObject *parent);
  virtual ~NameModel();

  void append(libmatroska::KaxChapterDisplay &display);
  void addNew();
  void remove(QModelIndex const &idx);
  void updateRow(int row);

  void populate(libmatroska::KaxChapterAtom &chapter);
  void reset();
  void retranslateUi();

  libmatroska::KaxChapterDisplay *displayFromIndex(QModelIndex const &idx);
  libmatroska::KaxChapterDisplay *displayFromItem(QStandardItem *item);

  virtual Qt::DropActions supportedDropActions() const override;
  virtual Qt::ItemFlags flags(QModelIndex const &index) const override;

  virtual bool canDropMimeData(QMimeData const *data, Qt::DropAction action, int row, int column, QModelIndex const &parent) const override;
  virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

protected:
  void setRowText(QList<QStandardItem *> const &rowItems);
  QList<QStandardItem *> itemsForRow(int row);
  qulonglong registerDisplay(libmatroska::KaxChapterDisplay &display);
  qulonglong registryIdFromItem(QStandardItem *item);

protected:
  static QList<QStandardItem *> newRowItems();
};

}}}
