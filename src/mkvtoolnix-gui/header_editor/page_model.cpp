#include "common/common_pch.h"

#include "common/qt.h"
#include "mkvtoolnix-gui/header_editor/page_base.h"
#include "mkvtoolnix-gui/header_editor/page_model.h"
#include "mkvtoolnix-gui/header_editor/top_level_page.h"
#include "mkvtoolnix-gui/util/model.h"

namespace mtx { namespace gui { namespace HeaderEditor {

using namespace mtx::gui;

PageModel::PageModel(QObject *parent)
  : QStandardItemModel{parent}
{
}

PageModel::~PageModel() {
}

PageBase *
PageModel::selectedPage(QModelIndex const &idx)
  const {
  auto selectedItem = itemFromIndex(idx.sibling(idx.row(), 0));
  if (!selectedItem)
    return {};

  auto pageId = selectedItem->data(Util::HeaderEditorPageIdRole).value<int>();
  return m_pages.value(pageId, nullptr);
}

void
PageModel::appendPage(PageBase *page,
                      QModelIndex const &parentIdx) {
  page->retranslateUi();

  auto pageId     = ++m_pageId;
  auto parentItem = parentIdx.isValid() ? itemFromIndex(parentIdx.sibling(parentIdx.row(), 0)) : invisibleRootItem();
  auto newItems   = QList<QStandardItem *>{};

  for (auto idx = columnCount(); idx > 0; --idx)
    newItems << new QStandardItem{};

  newItems[0]->setData(pageId, Util::HeaderEditorPageIdRole);

  parentItem->appendRow(newItems);

  page->m_pageIdx = indexFromItem(newItems[0]);
  page->setItems(newItems);

  m_pages[pageId] = page;
  if (!parentIdx.isValid())
    m_topLevelPages << page;
}

bool
PageModel::deletePage(PageBase *page) {
  auto pageId = m_pages.key(page);
  if (!pageId)
    return false;

  m_pages.remove(pageId);
  delete page;

  auto idx = m_topLevelPages.indexOf(page);
  if (-1 != idx)
    m_topLevelPages.removeAt(idx);

  return true;
}

QList<PageBase *>
PageModel::pages()
  const {
  auto pages = QList<PageBase *>{};
  for (auto const &page : m_pages)
    pages << page;

  return pages;
}

QList<PageBase *> const &
PageModel::topLevelPages()
  const {
  return m_topLevelPages;
}

QList<PageBase *>
PageModel::allExpandablePages()
  const {
  auto pages = m_topLevelPages;

  for (auto const &page : m_topLevelPages)
    for (auto const &subPage : page->m_children)
      if (dynamic_cast<TopLevelPage *>(subPage))
        pages << static_cast<TopLevelPage *>(subPage);

  return pages;
}

void
PageModel::reset() {
  beginResetModel();

  for (auto const &page : m_pages)
    delete page;

  m_pages.clear();
  m_topLevelPages.clear();

  removeRows(0, rowCount());

  endResetModel();
}

QModelIndex
PageModel::validate()
  const {
  for (auto page : m_topLevelPages) {
    auto result = page->validate();
    if (result.isValid())
      return result;
  }

  return QModelIndex{};
}

QList<QStandardItem *>
PageModel::itemsForIndex(QModelIndex const &idx) {
  auto items = QList<QStandardItem *>{};
  for (auto column = 0, numColumns = columnCount(); column < numColumns; ++column)
    items << itemFromIndex(idx.sibling(idx.row(), column));

  return items;
}

QModelIndex
PageModel::indexFromPage(PageBase *page)
  const {
  return Util::findIndex(*this, [this, page](QModelIndex const &idx) -> bool {
    return selectedPage(idx) == page;
  });
}

void
PageModel::retranslateUi() {
  Util::setDisplayableAndSymbolicColumnNames(*this, {
    { QY("Type"),             Q("type")             },
    { QY("Codec/MIME type"),  Q("codec")            },
    { QY("Language"),         Q("language")         },
    { QY("Name/Description"), Q("name")             },
    { QY("UID"),              Q("uid")              },
    { QY("Default track"),    Q("defaultTrackFlag") },
    { QY("Forced track"),     Q("forcedTrackFlag")  },
    { QY("Properties"),       Q("properties")       },
  });

  horizontalHeaderItem(4)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

  Util::walkTree(*this, QModelIndex{}, [=](QModelIndex const &currentIdx) {
    auto page = selectedPage(currentIdx);
    if (page)
      page->setItems(itemsForIndex(currentIdx));
  });
}

}}}
