#include "common/common_pch.h"

#include <QAction>
#include <QHeaderView>
#include <QMenu>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QTimer>

#include "common/qt.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/util/header_view_manager.h"
#include "mkvtoolnix-gui/util/model.h"
#include "mkvtoolnix-gui/util/settings.h"

namespace mtx { namespace gui { namespace Util {

class HeaderViewManagerPrivate {
  friend class HeaderViewManager;

  QHeaderView *headerView{};
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
HeaderViewManager::manage(QHeaderView &headerView,
                          QString const &name) {
  Q_D(HeaderViewManager);

  d->name       = name;
  d->headerView = &headerView;

  d->headerView->setContextMenuPolicy(Qt::CustomContextMenu);

  connect(d->headerView,     &QHeaderView::customContextMenuRequested, this, &HeaderViewManager::showContextMenu);
  connect(d->headerView,     &QHeaderView::sectionMoved,               this, &HeaderViewManager::saveState);

  QTimer::singleShot(0, this, SLOT(restoreState()));
}

void
HeaderViewManager::saveState() {
  Q_D(HeaderViewManager);

  if (d->restoringState)
    return;

  QStringList hiddenFlags, visualIndexes;

  for (auto logicalIndex = 0, columnCount = d->headerView->count(); logicalIndex < columnCount; ++logicalIndex) {
    hiddenFlags   << Q("%1").arg(d->headerView->isSectionHidden(logicalIndex));
    visualIndexes << Q("%1").arg(d->headerView->visualIndex(logicalIndex));
  }

  auto reg = Settings::registry();

  reg->beginGroup("headerViewManager");
  reg->beginGroup(d->name);

  reg->setValue("hidden",      hiddenFlags);
  reg->setValue("visualIndex", visualIndexes);

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

  restoreVisualIndexes(reg->value("visualIndex").toStringList());
  restoreHidden(reg->value("hidden").toStringList());

  reg->endGroup();
  reg->endGroup();

  d->restoringState = false;
}

void
HeaderViewManager::restoreHidden(QStringList hiddenFlags) {
  Q_D(HeaderViewManager);

  auto const columnCount = d->headerView->count();

  while (hiddenFlags.count() < columnCount)
    hiddenFlags << Q("0");

  for (auto logicalIndex = 0; logicalIndex < columnCount; ++logicalIndex)
    d->headerView->setSectionHidden(logicalIndex, !!hiddenFlags[logicalIndex].toInt());
}

void
HeaderViewManager::restoreVisualIndexes(QStringList visualIndexes) {
  Q_D(HeaderViewManager);

  auto visualToLogical   = QHash<int, int>{};
  auto const columnCount = d->headerView->count();

  while (visualIndexes.count() < columnCount)
    visualIndexes << Q("%1").arg(visualIndexes.count());

  for (int logicalIndex = columnCount - 1; logicalIndex >= 0; --logicalIndex) {
    auto const visualIndex = visualIndexes.value(logicalIndex, Q("%1").arg(logicalIndex)).toInt();
    visualToLogical[visualIndex] = logicalIndex;
  }

  for (int visualIndex = columnCount - 1; visualIndex > 0; --visualIndex) {
    if (!visualToLogical.contains(visualIndex))
      continue;

    auto logicalIndex = visualToLogical[visualIndex];
    if ((0 > logicalIndex) || (columnCount <= logicalIndex))
      continue;

    auto currentVisualIndex = d->headerView->visualIndex(logicalIndex);

    if (currentVisualIndex != visualIndex)
      d->headerView->moveSection(currentVisualIndex, visualIndex);
  }
}

HeaderViewManager *
HeaderViewManager::create(QHeaderView &headerView,
                          QString const &name) {
  auto manager = new HeaderViewManager{&headerView};
  manager->manage(headerView, name);

  return manager;
}

void
HeaderViewManager::toggleColumn(int column) {
  Q_D(HeaderViewManager);

  d->headerView->setSectionHidden(column, !d->headerView->isSectionHidden(column));

  saveState();
}

void
HeaderViewManager::showContextMenu(QPoint const &pos) {
  Q_D(HeaderViewManager);

  auto menu = new QMenu{d->headerView};

  for (int column = 1, columnCount = d->headerView->count(); column < columnCount; ++column) {
    auto action = new QAction{menu};
    auto text   = d->headerView->model()->headerData(column, Qt::Horizontal, Util::HiddenDescriptionRole).toString();

    if (text.isEmpty())
      text = d->headerView->model()->headerData(column, Qt::Horizontal, Qt::DisplayRole).toString();

    action->setText(text);
    action->setCheckable(true);
    action->setChecked(!d->headerView->isSectionHidden(column));

    menu->addAction(action);

    connect(action, &QAction::triggered, [=]() {
      toggleColumn(column);
    });
  }

  menu->exec(static_cast<QWidget *>(sender())->mapToGlobal(pos));
}

}}}
