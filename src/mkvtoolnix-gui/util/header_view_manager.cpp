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

  auto reg = Settings::registry();

  reg->beginGroup("headerViewManager");
  reg->setValue(d->name, d->headerView->saveState());
  reg->endGroup();
}

void
HeaderViewManager::restoreState() {
  Q_D(HeaderViewManager);

  auto reg = Settings::registry();

  reg->beginGroup("headerViewManager");

  auto state = reg->value(d->name).toByteArray();
  if (!state.isEmpty())
    d->headerView->restoreState(state);

  reg->endGroup();
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
