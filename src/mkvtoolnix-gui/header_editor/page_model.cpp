#include "common/common_pch.h"

#include "mkvtoolnix-gui/header_editor/page_base.h"
#include "mkvtoolnix-gui/header_editor/page_model.h"
#include "mkvtoolnix-gui/util/util.h"

namespace mtx { namespace gui { namespace HeaderEditor {

using namespace mtx::gui;

PageModel::PageModel(QObject *parent,
                     QtKaxAnalyzer &analyzer)
  : QStandardItemModel{parent}
  , m_analyzer{analyzer}
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
  newItem->setText(page->getTitle());

  parentItem->appendRow(newItem);

  page->m_pageIdx = indexFromItem(newItem);

  m_pages << page;
  if (!parentIdx.isValid())
    m_topLevelPages << page;
}

QList<PageBase *> &
PageModel::getPages() {
  return m_pages;
}

}}}
