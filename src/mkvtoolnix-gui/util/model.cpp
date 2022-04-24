#include "common/common_pch.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QIcon>
#include <QItemSelection>
#include <QItemSelectionModel>
#include <QStandardItemModel>
#include <QTreeView>

#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/util/model.h"

namespace mtx::gui::Util {

void
resizeViewColumnsToContents(QTreeView *view) {
  auto columnCount = view->model()->columnCount(QModelIndex{});
  for (auto column = 0; columnCount > column; ++column)
    view->resizeColumnToContents(column);
}

void
setDisplayableAndSymbolicColumnNames(QStandardItemModel &model,
                                     QList< std::pair<QString, QString> > const &columns) {
  auto headerLabels  = QStringList{};
  auto symbolicNames = QStringList{};

  for (auto const &pair : columns) {
    headerLabels  << pair.first;
    symbolicNames << pair.second;
  }

  model.setHorizontalHeaderLabels(headerLabels);
  Util::setSymbolicColumnNames(model, symbolicNames);
}

void
setSymbolicColumnNames(QAbstractItemModel &model,
                       QStringList const &names) {
  for (auto column = 0, numColumns = std::min<int>(model.columnCount(), names.count()); column < numColumns; ++column)
    model.setHeaderData(column, Qt::Horizontal, names[column], Util::SymbolicNameRole);
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

void
walkTree(QAbstractItemModel &model,
         QModelIndex const &idx,
         std::function<void(QModelIndex const &)> const &worker) {
  if (!idx.isValid()) {
    for (auto row = 0, numRows = model.rowCount(); row < numRows; ++row)
      walkTree(model, model.index(row, 0), worker);

    return;
  }

  worker(idx);

  for (auto row = 0, numRows = model.rowCount(idx); row < numRows; ++row)
    walkTree(model, model.index(row, 0, idx), worker);
}

void
requestAllItems(QStandardItemModel &model,
                QModelIndex const &parent) {
  for (int row = 0, numRows = model.rowCount(parent), numCols = model.columnCount(); row < numRows; ++row) {
    for (int col = 0; col < numCols; ++col)
      model.itemFromIndex(model.index(row, col, parent));

    requestAllItems(model, model.index(row, 0, parent));
  }
}

QModelIndex
findIndex(QAbstractItemModel const &model,
          std::function<bool(QModelIndex const &)> const &predicate,
          QModelIndex const &idx) {
  if (idx.isValid() && predicate(idx))
    return idx;

  for (auto row = 0, numRows = model.rowCount(idx); row < numRows; ++row) {
    auto result = findIndex(model, predicate, model.index(row, 0, idx));
    if (result.isValid())
      return result;
  }

  return {};
}

void
setItemForegroundColorDisabled(QList<QStandardItem *> const &items,
                               bool disabled) {
  auto palette = MainWindow::get()->palette();
  palette.setCurrentColorGroup(disabled ? QPalette::Disabled : QPalette::Normal);

  auto brush = palette.text();
  for (auto &item : items)
    item->setForeground(brush);
}

QIcon
fixStandardItemIcon(QIcon const &icon) {
#if defined(SYS_WINDOWS)
  // Workaround for a Qt bug on Windows: SVG icons used in item views
  // are stretched somehow. Raster icons (PNGs) don't have that
  // problem, though.
  auto newIcon = QIcon{icon.pixmap(16)};
  newIcon.addPixmap(icon.pixmap(24));
  newIcon.addPixmap(icon.pixmap(32));
  return newIcon;

#else  // SYS_WINDOWS
  return icon;
#endif  // SYS_WINDOWS
}

}
