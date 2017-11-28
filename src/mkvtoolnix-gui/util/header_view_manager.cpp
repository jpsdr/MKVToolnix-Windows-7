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

namespace mtx { namespace gui { namespace Util {

class HeaderViewManagerPrivate {
  friend class HeaderViewManager;

  QTreeView *treeView{};
  QString name;
  bool preventSaving{true};
  QTimer delayedSaveState{};
};

HeaderViewManager::HeaderViewManager(QObject *parent)
  : QObject{parent}
  , d_ptr{new HeaderViewManagerPrivate}
{
  d_ptr->delayedSaveState.setSingleShot(true);
}

HeaderViewManager::~HeaderViewManager() {
}

void
HeaderViewManager::manage(QTreeView &treeView,
                          QString const &name) {
  Q_D(HeaderViewManager);

  d->name     = name;
  d->treeView = &treeView;

  d->treeView->header()->setContextMenuPolicy(Qt::CustomContextMenu);

  QTimer::singleShot(0, this, SLOT(restoreState()));
}

void
HeaderViewManager::saveState() {
  Q_D(HeaderViewManager);

  if (d->preventSaving)
    return;

  QStringList hiddenColumns, columnOrder, columnSizes;
  QHash<QString, int> visualIndexMap;

  auto headerView = d->treeView->header();

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

  reg->beginGroup("headerViewManager");
  reg->beginGroup(d->name);

  reg->setValue("columnOrder",   columnOrder);
  reg->setValue("columnSizes",   columnSizes);
  reg->setValue("hiddenColumns", hiddenColumns);

  reg->endGroup();
  reg->endGroup();
}

void
HeaderViewManager::restoreState() {
  Q_D(HeaderViewManager);

  auto reg = Settings::registry();

  reg->beginGroup("headerViewManager");
  reg->beginGroup(d->name);

  restoreVisualIndexes(reg->value("columnOrder").toStringList());
  restoreHidden(reg->value("hiddenColumns").toStringList());
  restoreSizes(reg->value("columnSizes").toStringList());

  reg->endGroup();
  reg->endGroup();

  auto headerView = d->treeView->header();

  connect(headerView,           &QHeaderView::customContextMenuRequested, this, &HeaderViewManager::showContextMenu);
  connect(headerView,           &QHeaderView::sectionMoved,               this, &HeaderViewManager::saveState);
  connect(headerView,           &QHeaderView::sectionResized,             this, [d]() { d->delayedSaveState.start(200); });
  connect(&d->delayedSaveState, &QTimer::timeout,                         this, &HeaderViewManager::saveState);

  d->preventSaving = false;
}

void
HeaderViewManager::restoreHidden(QStringList const &hiddenColumns) {
  Q_D(HeaderViewManager);

  auto headerView        = d->treeView->header();
  auto const columnCount = headerView->count();

  for (auto logicalIndex = 0; logicalIndex < columnCount; ++logicalIndex)
    headerView->setSectionHidden(logicalIndex, hiddenColumns.contains(symbolicColumnName(logicalIndex)));
}

void
HeaderViewManager::restoreVisualIndexes(QStringList const &columnOrder) {
  Q_D(HeaderViewManager);

  auto headerView        = d->treeView->header();
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

  for (int visualIndex = columnCount - 1; visualIndex > 0; --visualIndex) {
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
  Q_D(HeaderViewManager);

  auto headerView  = d->treeView->header();
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
    auto name = symbolicColumnName(logicalIndex);
    if (sizeMap.contains(name) && !headerView->isSectionHidden(logicalIndex))
      headerView->resizeSection(logicalIndex, sizeMap[name]);
  }
}

HeaderViewManager *
HeaderViewManager::create(QTreeView &treeView,
                          QString const &name) {
  auto manager = new HeaderViewManager{&treeView};
  manager->manage(treeView, name);

  return manager;
}

void
HeaderViewManager::toggleColumn(int column) {
  Q_D(HeaderViewManager);

  auto headerView = d->treeView->header();
  headerView->setSectionHidden(column, !headerView->isSectionHidden(column));

  saveState();

  resizeViewColumnsToContents(d->treeView);
}

void
HeaderViewManager::resetColumns() {
  Q_D(HeaderViewManager);

  d->preventSaving = true;

  restoreVisualIndexes({});
  restoreHidden({});

  d->preventSaving = false;

  saveState();

  resizeViewColumnsToContents(d->treeView);
}

void
HeaderViewManager::showContextMenu(QPoint const &pos) {
  Q_D(HeaderViewManager);

  auto headerView = d->treeView->header();
  auto menu       = new QMenu{headerView};
  auto action     = new QAction{menu};

  action->setText(QY("Reset all columns"));
  menu->addAction(action);
  menu->addSeparator();

  connect(action, &QAction::triggered, this, &HeaderViewManager::resetColumns);

  for (int column = 1, columnCount = headerView->count(); column < columnCount; ++column) {
    auto action = new QAction{menu};
    auto text   = headerView->model()->headerData(column, Qt::Horizontal, Util::HiddenDescriptionRole).toString();

    if (text.isEmpty())
      text = headerView->model()->headerData(column, Qt::Horizontal, Qt::DisplayRole).toString();

    action->setText(text);
    action->setCheckable(true);
    action->setChecked(!headerView->isSectionHidden(column));

    menu->addAction(action);

    connect(action, &QAction::triggered, [=]() {
      toggleColumn(column);
    });
  }

  menu->exec(static_cast<QWidget *>(sender())->mapToGlobal(pos));
}

QString
HeaderViewManager::symbolicColumnName(int logicalIndex) {
  Q_D(HeaderViewManager);

  return d->treeView->model()->headerData(logicalIndex, Qt::Horizontal, Util::SymbolicNameRole).toString();
}

}}}
