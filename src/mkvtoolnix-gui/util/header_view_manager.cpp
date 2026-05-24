#include "common/common_pch.h"

#include <QAction>
#include <QHeaderView>
#include <QMenu>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QTreeView>

#include "common/qt.h"
#include "common/sorting.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/util/header_view_manager.h"
#include "mkvtoolnix-gui/util/model.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/settings_names.h"

namespace mtx::gui::Util {

class HeaderViewManagerPrivate {
  friend class HeaderViewManager;

  QTreeView *treeView{};
  QString name;
  bool preventSaving{true};
  QTimer delayedSaveState{};
  QHash<QString, int> defaultSizes;
};

HeaderViewManager::HeaderViewManager(QObject *parent)
  : QObject{parent}
  , p_ptr{new HeaderViewManagerPrivate}
{
  p_func()->delayedSaveState.setSingleShot(true);
}

HeaderViewManager::~HeaderViewManager() {
}

void
HeaderViewManager::manage(QTreeView &treeView,
                          QString const &name) {
  auto p      = p_func();
  p->name     = name;
  p->treeView = &treeView;

  p->treeView->header()->setContextMenuPolicy(Qt::CustomContextMenu);

  QTimer::singleShot(0, this, [this]() { restoreState(); });
}

HeaderViewManager &
HeaderViewManager::setDefaultSizes(QHash<QString, int> const &defaultSizes) {
  p_func()->defaultSizes = defaultSizes;
  return *this;
}

void
HeaderViewManager::saveState() {
  auto p = p_func();

  if (p->preventSaving)
    return;

  QStringList hiddenColumns, columnOrder, columnSizes;
  QHash<QString, int> visualIndexMap;

  auto headerView = p->treeView->header();

  for (auto logicalIndex = 0, columnCount = headerView->count(); logicalIndex < columnCount; ++logicalIndex) {
    auto columnName = symbolicColumnName(logicalIndex);

    columnOrder << columnName;
    visualIndexMap[columnName] = headerView->visualIndex(logicalIndex);

    if (headerView->isSectionHidden(logicalIndex))
      hiddenColumns << columnName;

    else
      columnSizes << Q("%1:%2").arg(columnName).arg(headerView->sectionSize(logicalIndex));
  }

  mtx::sort::by(columnOrder.begin(), columnOrder.end(), [&visualIndexMap](QString const &columnName) { return visualIndexMap[columnName]; });

  auto reg = Settings::registry();

  reg->beginGroup(s_grpHeaderViewManager);
  reg->beginGroup(p->name);

  reg->setValue(s_valColumnOrder,   columnOrder);
  reg->setValue(s_valColumnSizes,   columnSizes);
  reg->setValue(s_valHiddenColumns, hiddenColumns);

  reg->endGroup();
  reg->endGroup();
}

void
HeaderViewManager::restoreState() {
  auto p   = p_func();
  auto reg = Settings::registry();

  reg->beginGroup(s_grpHeaderViewManager);
  reg->beginGroup(p->name);

  restoreVisualIndexes(reg->value(s_valColumnOrder).toStringList());
  restoreHidden(reg->value(s_valHiddenColumns).toStringList());
  restoreSizes(reg->value(s_valColumnSizes).toStringList());

  reg->endGroup();
  reg->endGroup();

  auto headerView = p->treeView->header();

  connect(headerView,           &QHeaderView::customContextMenuRequested, this, &HeaderViewManager::showContextMenu);
  connect(headerView,           &QHeaderView::sectionMoved,               this, &HeaderViewManager::saveState);
  connect(headerView,           &QHeaderView::sectionResized,             this, [p]() { p->delayedSaveState.start(200); });
  connect(&p->delayedSaveState, &QTimer::timeout,                         this, &HeaderViewManager::saveState);

  p->preventSaving = false;
}

void
HeaderViewManager::restoreHidden(QStringList const &hiddenColumns) {
  auto p                 = p_func();
  auto headerView        = p->treeView->header();
  auto const columnCount = headerView->count();

  for (auto logicalIndex = 0; logicalIndex < columnCount; ++logicalIndex)
    headerView->setSectionHidden(logicalIndex, hiddenColumns.contains(symbolicColumnName(logicalIndex)));
}

void
HeaderViewManager::restoreVisualIndexes(QStringList const &columnOrder) {
  auto p                 = p_func();
  auto headerView        = p->treeView->header();
  auto visualIndexes     = QHash<QString, int>{};
  auto visualToLogical   = QHash<int, int>{};
  auto const columnCount = headerView->count();
  auto visualIndex       = 0;

  for (auto const &columnName : columnOrder)
    visualIndexes[columnName] = visualIndex++;

  for (auto logicalIndex = 0; logicalIndex < columnCount; ++logicalIndex) {
    auto const columnName = symbolicColumnName(logicalIndex);
    if (!visualIndexes.contains(columnName))
      visualIndexes[columnName] = visualIndex++;
  }

  for (auto logicalIndex = 0; logicalIndex < columnCount; ++logicalIndex) {
    auto const columnName        = symbolicColumnName(logicalIndex);
    visualIndex                  = visualIndexes[columnName];
    visualToLogical[visualIndex] = logicalIndex;
  }

  for (visualIndex = columnCount - 1; visualIndex > 0; --visualIndex) {
    if (!visualToLogical.contains(visualIndex))
      continue;

    auto logicalIndex = visualToLogical[visualIndex];
    if ((0 > logicalIndex) || (columnCount <= logicalIndex))
      continue;

    auto currentVisualIndex = headerView->visualIndex(logicalIndex);

    if (currentVisualIndex != visualIndex)
      headerView->moveSection(currentVisualIndex, visualIndex);
  }
}

void
HeaderViewManager::restoreSizes(QStringList const &columnSizes) {
  auto p           = p_func();
  auto headerView  = p->treeView->header();
  auto columnCount = headerView->count();

  QHash<QString, int> sizeMap;
  for (auto const &pair : columnSizes) {
    auto items = pair.split(Q(":"));
    sizeMap[items[0]] = items[1].toInt();
  }

  QVector<int> logicalIndexesInVisualOrder;
  for (int logicalIndex = 0; logicalIndex < columnCount; ++logicalIndex)
    logicalIndexesInVisualOrder << logicalIndex;

  mtx::sort::by(logicalIndexesInVisualOrder.begin(), logicalIndexesInVisualOrder.end(), [headerView](int logicalIndex) { return headerView->visualIndex(logicalIndex); });

  for (int logicalIndex : logicalIndexesInVisualOrder) {
    if (headerView->isSectionHidden(logicalIndex))
      continue;

    auto name = symbolicColumnName(logicalIndex);
    auto size = sizeMap.contains(name)         ? sizeMap[name]
              : p->defaultSizes.contains(name) ? p->defaultSizes[name]
              :                                  0;

    if (size != 0)
      headerView->resizeSection(logicalIndex, size);
  }
}

HeaderViewManager &
HeaderViewManager::create(QTreeView &treeView,
                          QString const &name) {
  auto manager = new HeaderViewManager{&treeView};
  manager->manage(treeView, name);

  return *manager;
}

void
HeaderViewManager::toggleColumn(int column) {
  auto p          = p_func();
  auto headerView = p->treeView->header();

  headerView->setSectionHidden(column, !headerView->isSectionHidden(column));

  saveState();

  resizeViewColumnsToContents(p->treeView);
}

void
HeaderViewManager::resetColumns() {
  auto p           = p_func();
  p->preventSaving = true;

  restoreVisualIndexes({});
  restoreHidden({});

  p->preventSaving = false;

  saveState();

  resizeViewColumnsToContents(p->treeView);
}

void
HeaderViewManager::showContextMenu(QPoint const &pos) {
  auto p          = p_func();
  auto headerView = p->treeView->header();
  auto menu       = new QMenu{headerView};
  auto action     = new QAction{menu};

  action->setText(QY("Reset all columns"));
  menu->addAction(action);
  menu->addSeparator();

  connect(action, &QAction::triggered, this, &HeaderViewManager::resetColumns);

  for (int column = 1, columnCount = headerView->count(); column < columnCount; ++column) {
    action    = new QAction{menu};
    auto text = headerView->model()->headerData(column, Qt::Horizontal, Util::HiddenDescriptionRole).toString();

    if (text.isEmpty())
      text = headerView->model()->headerData(column, Qt::Horizontal, Qt::DisplayRole).toString();

    action->setText(text);
    action->setCheckable(true);
    action->setChecked(!headerView->isSectionHidden(column));

    menu->addAction(action);

    connect(action, &QAction::triggered, [this, column]() {
      toggleColumn(column);
    });
  }

  menu->exec(static_cast<QWidget *>(sender())->mapToGlobal(pos));
}

QString
HeaderViewManager::symbolicColumnName(int logicalIndex) {
  return p_func()->treeView->model()->headerData(logicalIndex, Qt::Horizontal, Util::SymbolicNameRole).toString();
}

}
