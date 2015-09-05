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
  bool restoringState{};
};

HeaderViewManager::HeaderViewManager(QObject *parent)
  : QObject{parent}
  , d_ptr{new HeaderViewManagerPrivate}
{
}

HeaderViewManager::~HeaderViewManager() {
}

void
HeaderViewManager::manage(QTreeView &treeView,
                          QString const &name) {
  Q_D(HeaderViewManager);

  d->name         = name;
  d->treeView     = &treeView;

  auto headerView = d->treeView->header();

  headerView->setContextMenuPolicy(Qt::CustomContextMenu);

  connect(headerView, &QHeaderView::customContextMenuRequested, this, &HeaderViewManager::showContextMenu);
  connect(headerView, &QHeaderView::sectionMoved,               this, &HeaderViewManager::saveState);

  QTimer::singleShot(0, this, SLOT(restoreState()));
}

void
HeaderViewManager::saveState() {
  Q_D(HeaderViewManager);

  if (d->restoringState)
    return;

  QStringList hiddenColumns, columnOrder;
  QHash<QString, int> visualIndexMap;

  auto headerView = d->treeView->header();

  for (auto logicalIndex = 0, columnCount = headerView->count(); logicalIndex < columnCount; ++logicalIndex) {
    auto columnName = symbolicColumnName(logicalIndex);

    columnOrder << columnName;
    visualIndexMap[columnName] = headerView->visualIndex(logicalIndex);

    if (headerView->isSectionHidden(logicalIndex))
      hiddenColumns << columnName;
  }

  mtx::sort::by(columnOrder.begin(), columnOrder.end(), [&visualIndexMap](QString const &columnName) { return visualIndexMap[columnName]; });

  auto reg = Settings::registry();

  reg->beginGroup("headerViewManager");
  reg->beginGroup(d->name);

  reg->setValue("columnOrder",   columnOrder);
  reg->setValue("hiddenColumns", hiddenColumns);

  reg->endGroup();
  reg->endGroup();
}

void
HeaderViewManager::restoreState() {
  Q_D(HeaderViewManager);

  d->restoringState = true;

  auto reg = Settings::registry();

  reg->beginGroup("headerViewManager");
  reg->beginGroup(d->name);

  restoreVisualIndexes(reg->value("columnOrder").toStringList());
  restoreHidden(reg->value("hiddenColumns").toStringList());

  reg->endGroup();
  reg->endGroup();

  d->restoringState = false;
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

  d->restoringState = true;

  restoreVisualIndexes({});
  restoreHidden({});

  d->restoringState = false;

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
