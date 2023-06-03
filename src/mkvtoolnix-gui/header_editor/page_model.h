#pragma once

#include "common/common_pch.h"

#include <QList>
#include <QStandardItemModel>

class QAbstractItemView;
class QMimeData;

namespace mtx::gui::HeaderEditor {

class PageBase;

class PageModel: public QStandardItemModel {
  Q_OBJECT
protected:
  QHash<int, PageBase *> m_pages;
  int m_pageId{};
  QModelIndex m_lastSelectedIdx;

public:
  static const int TypeColumn          = 0;
  static const int CodecColumn         = 1;
  static const int LanguageColumn      = 2;
  static const int NameColumn          = 3;
  static const int UidColumn           = 4;
  static const int DefaultTrackColumn  = 5;
  static const int ForcedDisplayColumn = 6;
  static const int EnabledColumn       = 7;
  static const int PropertiesColumn    = 8;

public:
  PageModel(QObject *parent);
  virtual ~PageModel();

  PageBase *selectedPage(QModelIndex const &idx) const;

  void appendPage(PageBase *page, QModelIndex const &parentIdx = {});
  bool deletePage(PageBase *page);

  QList<PageBase *> pages() const;
  QList<PageBase *> topLevelPages() const;
  QList<PageBase *> allExpandablePages() const;

  void reset();

  QModelIndex validate() const;

  void retranslateUi();

  QList<QStandardItem *> itemsForIndex(QModelIndex const &idx);
  QModelIndex indexFromPage(PageBase *page) const;
  QModelIndex trackOrAttachedFileIndexForSelectedIndex(QModelIndex const &idx);

  void rememberLastSelectedIndex(QModelIndex const &idx);

  virtual bool canDropMimeData(QMimeData const *data, Qt::DropAction action, int row, int, QModelIndex const &parent) const override;
  virtual bool dropMimeData(QMimeData const *data, Qt::DropAction action, int row, int column, QModelIndex const &parent) override;

  void moveElementUpOrDown(QModelIndex const &idx, bool up);

Q_SIGNALS:
  void attachmentsReordered();
  void tracksReordered();

public Q_SLOTS:
  void rereadTopLevelPageIndexes();
};

}
