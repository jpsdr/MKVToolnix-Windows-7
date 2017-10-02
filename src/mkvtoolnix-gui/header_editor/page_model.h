#pragma once

#include "common/common_pch.h"

#include <QList>
#include <QStandardItemModel>

class QAbstractItemView;

namespace mtx { namespace gui { namespace HeaderEditor {

class PageBase;

class PageModel: public QStandardItemModel {
  Q_OBJECT;
protected:
  QHash<int, PageBase *> m_pages;
  QList<PageBase *> m_topLevelPages;
  int m_pageId{};

public:
  PageModel(QObject *parent);
  virtual ~PageModel();

  PageBase *selectedPage(QModelIndex const &idx) const;

  void appendPage(PageBase *page, QModelIndex const &parentIdx = {});
  bool deletePage(PageBase *page);

  QList<PageBase *> pages() const;
  QList<PageBase *> const &topLevelPages() const;
  QList<PageBase *> allExpandablePages() const;

  void reset();

  QModelIndex validate() const;

  void retranslateUi();

  QList<QStandardItem *> itemsForIndex(QModelIndex const &idx);
  QModelIndex indexFromPage(PageBase *page) const;
};

}}}
