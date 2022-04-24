#pragma once

#include "common/common_pch.h"

class QAbstractItemModel;
class QAbstractItemView;
class QIcon;
class QItemSelection;
class QItemSelectionModel;
class QStandardItem;
class QStandardItemModel;
class QTreeView;

namespace mtx::gui::Util {

// Model stuff
enum MtxGuiRoles {
  SourceFileRole = Qt::UserRole + 1,
  TrackRole,
  JobIdRole,
  HeaderEditorPageIdRole,
  ChapterEditorChapterOrEditionRole,
  ChapterEditorChapterDisplayRole,
  AttachmentRole,
  HiddenDescriptionRole,
  SymbolicNameRole,
  SortRole,
};

void resizeViewColumnsToContents(QTreeView *view);
void setSymbolicColumnNames(QAbstractItemModel &model, QStringList const &names);
void setDisplayableAndSymbolicColumnNames(QStandardItemModel &model, QList< std::pair<QString, QString> > const &columns);
int numSelectedRows(QItemSelection &selection);
QModelIndex selectedRowIdx(QItemSelection const &selection);
QModelIndex selectedRowIdx(QAbstractItemView *view);
void withSelectedIndexes(QItemSelectionModel *selectionModel, std::function<void(QModelIndex const &)> worker);
void withSelectedIndexes(QAbstractItemView *view, std::function<void(QModelIndex const &)> worker);
void selectRow(QAbstractItemView *view, int row, QModelIndex const &parentIdx = QModelIndex{});
QModelIndex toTopLevelIdx(QModelIndex const &idx);
void walkTree(QAbstractItemModel &model, QModelIndex const &idx, std::function<void(QModelIndex const &)> const &worker);
void requestAllItems(QStandardItemModel &model, QModelIndex const &parent = QModelIndex{});
QModelIndex findIndex(QAbstractItemModel const &model, std::function<bool(QModelIndex const &)> const &predicate, QModelIndex const &idx = QModelIndex{});
void setItemForegroundColorDisabled(QList<QStandardItem *> const &items, bool disabled);

QIcon fixStandardItemIcon(QIcon const &icon);

}
