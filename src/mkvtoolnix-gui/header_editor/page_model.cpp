#include "common/common_pch.h"

#include <QMimeData>

#include "common/qt.h"
#include "mkvtoolnix-gui/header_editor/attached_file_page.h"
#include "mkvtoolnix-gui/header_editor/attachments_page.h"
#include "mkvtoolnix-gui/header_editor/page_base.h"
#include "mkvtoolnix-gui/header_editor/page_model.h"
#include "mkvtoolnix-gui/header_editor/top_level_page.h"
#include "mkvtoolnix-gui/header_editor/track_type_page.h"
#include "mkvtoolnix-gui/util/model.h"

namespace mtx::gui::HeaderEditor {

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
  if (!idx.isValid())
    return {};

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
}

bool
PageModel::deletePage(PageBase *page) {
  auto pageId = m_pages.key(page);
  if (!pageId)
    return false;

  m_pages.remove(pageId);
  delete page;

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

QList<PageBase *>
PageModel::topLevelPages()
  const {
  auto rootItem = invisibleRootItem();

  QList<PageBase *> pages;
  pages.reserve(rootItem->rowCount());

  for (int row = 0, numRows = rootItem->rowCount(); row < numRows; ++row) {
    auto topLevelItem = rootItem->child(row);
    auto pageId       = topLevelItem->data(Util::HeaderEditorPageIdRole).value<int>();

    pages << m_pages[pageId];
  }

  return pages;
}

QList<PageBase *>
PageModel::allExpandablePages()
  const {
  auto allTopLevelPages = topLevelPages();
  auto expandablePages  = allTopLevelPages;

  for (auto const &page : allTopLevelPages)
    for (auto const &subPage : page->m_children)
      if (dynamic_cast<TopLevelPage *>(subPage))
        expandablePages << static_cast<TopLevelPage *>(subPage);

  return expandablePages;
}

void
PageModel::reset() {
  beginResetModel();

  for (auto const &page : m_pages)
    delete page;

  m_pages.clear();

  removeRows(0, rowCount());

  endResetModel();
}

QModelIndex
PageModel::validate()
  const {
  for (auto page : topLevelPages()) {
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
    { QY("Forced display"),   Q("forcedTrackFlag")  },
    { QY("Enabled"),          Q("enabled")          },
    { QY("Properties"),       Q("properties")       },
  });

  horizontalHeaderItem(4)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

  Util::walkTree(*this, QModelIndex{}, [=](QModelIndex const &currentIdx) {
    auto page = selectedPage(currentIdx);
    if (page)
      page->setItems(itemsForIndex(currentIdx));
  });
}

void
PageModel::rememberLastSelectedIndex(QModelIndex const &idx) {
  m_lastSelectedIdx = idx;
}

bool
PageModel::canDropMimeData(QMimeData const *data,
                           Qt::DropAction action,
                           int row,
                           int,
                           QModelIndex const &parent)
  const {
  if (   !data
      || (Qt::MoveAction != action)
      || !m_lastSelectedIdx.isValid()
      || (0 > row))
    return false;

  auto draggedPage = selectedPage(m_lastSelectedIdx);
  auto parentPage  = selectedPage(parent);

  if (dynamic_cast<AttachedFilePage *>(draggedPage))
    return dynamic_cast<AttachmentsPage *>(parentPage);

  if (dynamic_cast<TrackTypePage *>(draggedPage))
    return !parentPage && (row > 0) && (row < rowCount());

  return false;
}

bool
PageModel::dropMimeData(QMimeData const *data,
                        Qt::DropAction action,
                        int row,
                        int column,
                        QModelIndex const &parent) {
  if (!canDropMimeData(data, action, row, column, parent))
    return false;

  auto draggedPage = selectedPage(m_lastSelectedIdx);

  auto result = QStandardItemModel::dropMimeData(data, action, row, 0, parent);

  Util::requestAllItems(*this);

  if (dynamic_cast<AttachedFilePage *>(draggedPage))
    Q_EMIT attachmentsReordered();

  else if (dynamic_cast<TrackTypePage *>(draggedPage))
    Q_EMIT tracksReordered();

  return result;
}

void
PageModel::rereadTopLevelPageIndexes() {
  auto rootItem = invisibleRootItem();

  for (int row = 0, numRows = rootItem->rowCount(); row < numRows; ++row) {
    auto topLevelItem          = rootItem->child(row);
    auto pageId                = topLevelItem->data(Util::HeaderEditorPageIdRole).value<int>();
    m_pages[pageId]->m_pageIdx = topLevelItem->index();
  }
}

void
PageModel::moveElementUpOrDown(QModelIndex const &idx,
                               bool up) {

  auto page = selectedPage(idx);
  if (!idx.isValid() || !page)
    return;

  auto const isTrack    = !!dynamic_cast<TrackTypePage *>(page);
  auto const lowerLimit = isTrack ? 1 : 0;
  auto const upperLimit = rowCount(idx.parent()) - (isTrack ? 2 : 1);
  auto const newRow     = idx.row() + (up ? -1 : 1);

  if (up && (idx.row() <= lowerLimit))
    return;

  if (!up && (idx.row() >= upperLimit))
    return;

  auto parentItem = itemFromIndex(idx.parent());
  if (!parentItem)
    parentItem = invisibleRootItem();

  auto row = parentItem->takeRow(idx.row());
  parentItem->insertRow(newRow, row);
}

QModelIndex
PageModel::trackOrAttachedFileIndexForSelectedIndex(QModelIndex const &selectedIdx) {
  auto idx = selectedIdx;

  while (idx.isValid()) {
    auto page = selectedPage(idx);
    if (!page)
      return {};

    if (dynamic_cast<TrackTypePage *>(page) || dynamic_cast<AttachedFilePage *>(page))
      break;

    idx = idx.parent();
  }

  return idx;
}

}
