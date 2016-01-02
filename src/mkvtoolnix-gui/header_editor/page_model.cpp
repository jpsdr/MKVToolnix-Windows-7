#include "common/common_pch.h"

#include "common/qt.h"
#include "mkvtoolnix-gui/header_editor/page_base.h"
#include "mkvtoolnix-gui/header_editor/page_model.h"
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
  if (selectedItem)
    return m_pages[ selectedItem->data(Util::HeaderEditorPageIdRole).value<unsigned int>() ];

  return nullptr;
}

void
PageModel::appendPage(PageBase *page,
                      QModelIndex const &parentIdx) {
  page->retranslateUi();

  auto parentItem = parentIdx.isValid() ? itemFromIndex(parentIdx.sibling(parentIdx.row(), 0)) : invisibleRootItem();
  auto newItems   = QList<QStandardItem *>{};

  for (auto idx = columnCount(); idx > 0; --idx)
    newItems << new QStandardItem{};

  newItems[0]->setData(static_cast<unsigned int>(m_pages.count()), Util::HeaderEditorPageIdRole);

  parentItem->appendRow(newItems);

  page->m_pageIdx = indexFromItem(newItems[0]);
  page->setItems(newItems);

  m_pages << page;
  if (!parentIdx.isValid())
    m_topLevelPages << page;
}

bool
PageModel::deletePage(PageBase *page) {
  auto idx = m_pages.indexOf(page);
  if (idx == -1)
    return false;

  m_pages.removeAt(idx);
  delete page;

  return true;
}

QList<PageBase *> &
PageModel::pages() {
  return m_pages;
}

QList<PageBase *> &
PageModel::topLevelPages() {
  return m_topLevelPages;
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
