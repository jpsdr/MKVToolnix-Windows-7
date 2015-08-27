#include "common/common_pch.h"

#include <QAbstractItemView>
#include <QItemSelection>
#include <QItemSelectionModel>
#include <QTableView>
#include <QTreeView>

#include "mkvtoolnix-gui/util/model.h"

namespace mtx { namespace gui { namespace Util {

void
resizeViewColumnsToContents(QTableView *view) {
  auto columnCount = view->model()->columnCount(QModelIndex{});
  for (auto column = 0; columnCount > column; ++column)
    view->resizeColumnToContents(column);
}

void
resizeViewColumnsToContents(QTreeView *view) {
  auto columnCount = view->model()->columnCount(QModelIndex{});
  for (auto column = 0; columnCount > column; ++column)
    view->resizeColumnToContents(column);
}

void
withSelectedIndexes(QAbstractItemView *view,
                    std::function<void(QModelIndex const &)> worker) {
  withSelectedIndexes(view->selectionModel(), worker);
}

void
withSelectedIndexes(QItemSelectionModel *selectionModel,
                    std::function<void(QModelIndex const &)> worker) {
  auto rowsSeen = QMap< std::pair<QModelIndex, int>, bool >{};
  for (auto const &range : selectionModel->selection())
    for (auto const &index : range.indexes()) {
      auto seenIdx = std::make_pair(index.parent(), index.row());
      if (rowsSeen[seenIdx])
        continue;
      rowsSeen[seenIdx] = true;
      worker(index.sibling(index.row(), 0));
    }
}

int
numSelectedRows(QItemSelection &selection) {
  auto rowsSeen = QMap< std::pair<QModelIndex, int>, bool >{};
  for (auto const &range : selection)
    for (auto const &index : range.indexes()) {
      auto seenIdx      = std::make_pair(index.parent(), index.row());
      rowsSeen[seenIdx] = true;
    }

  return rowsSeen.count();
}

QModelIndex
selectedRowIdx(QItemSelection const &selection) {
  if (selection.isEmpty())
    return {};

  auto indexes = selection.at(0).indexes();
  if (indexes.isEmpty() || !indexes.at(0).isValid())
    return {};

  auto idx = indexes.at(0);
  return idx.sibling(idx.row(), 0);
}

QModelIndex
selectedRowIdx(QAbstractItemView *view) {
  if (!view)
    return {};
  return selectedRowIdx(view->selectionModel()->selection());
}

void
selectRow(QAbstractItemView *view,
          int row,
          QModelIndex const &parentIdx) {
  auto itemModel      = view->model();
  auto selectionModel = view->selectionModel();
  auto selection      = QItemSelection{itemModel->index(row, 0, parentIdx), itemModel->index(row, itemModel->columnCount() - 1, parentIdx)};
  selectionModel->select(selection, QItemSelectionModel::ClearAndSelect);
}

QModelIndex
toTopLevelIdx(QModelIndex const &idx) {
  if (!idx.isValid())
    return QModelIndex{};

  auto parent = idx.parent();
  return parent == QModelIndex{} ? idx : parent;
}

}}}
