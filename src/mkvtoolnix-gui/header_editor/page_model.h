#ifndef MTX_MKVTOOLNIX_GUI_HEADER_EDITOR_PAGE_MODEL_H
#define MTX_MKVTOOLNIX_GUI_HEADER_EDITOR_PAGE_MODEL_H

#include "common/common_pch.h"

#include <QList>
#include <QStandardItemModel>

class QAbstractItemView;

namespace mtx { namespace gui { namespace HeaderEditor {

class PageBase;

class PageModel: public QStandardItemModel {
  Q_OBJECT;
protected:
  QList<PageBase *> m_pages, m_topLevelPages;

public:
  PageModel(QObject *parent);
  virtual ~PageModel();

  PageBase *selectedPage(QModelIndex const &idx) const;

  void appendPage(PageBase *page, QModelIndex const &parentIdx = {});

  QList<PageBase *> &pages();
  QList<PageBase *> &topLevelPages();

  void reset();

  QModelIndex validate() const;
};

}}}

#endif  // MTX_MKVTOOLNIX_GUI_HEADER_EDITOR_PAGE_MODEL_H
