#include "common/common_pch.h"

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
  auto selectedItem = itemFromIndex(idx);
  if (!selectedItem)
    return nullptr;

  return m_pages[ data(idx, Util::HeaderEditorPageIdRole).value<unsigned int>() ];
}

void
PageModel::appendPage(PageBase *page,
                      QModelIndex const &parentIdx) {
  page->retranslateUi();

  auto parentItem = parentIdx.isValid() ? itemFromIndex(parentIdx) : invisibleRootItem();
  auto newItem    = new QStandardItem{};

  newItem->setData(static_cast<unsigned int>(m_pages.count()), Util::HeaderEditorPageIdRole);
  newItem->setText(page->title());

  parentItem->appendRow(newItem);

  page->m_pageIdx = indexFromItem(newItem);

  m_pages << page;
  if (!parentIdx.isValid())
    m_topLevelPages << page;
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

}}}
