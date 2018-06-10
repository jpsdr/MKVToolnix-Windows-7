#include "common/common_pch.h"

#include <QListWidget>
#include <QPushButton>

#include "common/qt.h"
#include "mkvtoolnix-gui/forms/util/side_by_side_multi_select.h"
#include "mkvtoolnix-gui/util/side_by_side_multi_select.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx { namespace gui { namespace Util {

class SideBySideMultiSelectPrivate {
  friend class SideBySideMultiSelect;

  std::unique_ptr<Ui::SideBySideMultiSelect> ui{new Ui::SideBySideMultiSelect};
  SideBySideMultiSelect::ItemList initiallyAvailableItems;
  QStringList initiallySelectedItems;
};

SideBySideMultiSelect::SideBySideMultiSelect(QWidget *parent)
  : QWidget{parent}
  , p_ptr{new SideBySideMultiSelectPrivate}
{
  auto p = p_func();

  p->ui->setupUi(this);

  p->ui->moveToAvailableButton->setEnabled(false);
  p->ui->moveToSelectedButton->setEnabled(false);

  setupConnections();
}

SideBySideMultiSelect::~SideBySideMultiSelect() {
}

void
SideBySideMultiSelect::retranslateUi() {
  p_func()->ui->retranslateUi(this);
}

void
SideBySideMultiSelect::setToolTips(QString const &available,
                                   QString const &selected) {
  auto p = p_func();

  Util::setToolTip(p->ui->available, available);
  Util::setToolTip(p->ui->selected,  selected.isEmpty() ? available : selected);
}

void
SideBySideMultiSelect::setupConnections() {
  auto p = p_func();

  connect(p->ui->available->selectionModel(), &QItemSelectionModel::selectionChanged, this, &SideBySideMultiSelect::availableSelectionChanged);
  connect(p->ui->selected->selectionModel(),  &QItemSelectionModel::selectionChanged, this, &SideBySideMultiSelect::selectedSelectionChanged);

  connect(p->ui->moveToAvailableButton,       &QPushButton::clicked,                  this, &SideBySideMultiSelect::moveToAvailable);
  connect(p->ui->moveToSelectedButton,        &QPushButton::clicked,                  this, &SideBySideMultiSelect::moveToSelected);
  connect(p->ui->revertButton,                &QPushButton::clicked,                  this, &SideBySideMultiSelect::revertLists);
}

void
SideBySideMultiSelect::setItems(ItemList const &all,
                                QStringList const &selected) {
  auto p = p_func();

  p->initiallyAvailableItems = all;
  p->initiallySelectedItems  = selected;

  p->ui->available->clear();
  p->ui->selected->clear();

  auto isSelected = QHash<QString, bool>{};

  for (auto const &item : selected)
    isSelected[item] = true;

  for (auto const &item : all) {
    auto addTo  = isSelected[item.second] ? p->ui->selected : p->ui->available;
    auto lwItem = new QListWidgetItem{item.first};

    lwItem->setData(Qt::UserRole, item.second);
    addTo->addItem(lwItem);
  }

  p->ui->moveToAvailableButton->setEnabled(false);
  p->ui->moveToSelectedButton->setEnabled(false);
}

void
SideBySideMultiSelect::setItems(QStringList const &all,
                                QStringList const &selected) {
  auto items = ItemList{};
  items.reserve(all.count());

  for (auto const &item : all)
    items << std::make_pair(item, item);

  setItems(items, selected);
}

SideBySideMultiSelect::ItemList
SideBySideMultiSelect::selectedItems()
  const {
  auto p = p_func();

  auto list = ItemList{};

  for (auto row = 0, numRows = p->ui->selected->count(); row < numRows; ++row) {
    auto item = p->ui->selected->item(row);
    list << std::make_pair(item->text(), item->data(Qt::UserRole).toString());
  }

  return list;
}

QStringList
SideBySideMultiSelect::selectedItemValues()
  const {
  auto p = p_func();

  auto list = QStringList{};

  for (auto row = 0, numRows = p->ui->selected->count(); row < numRows; ++row)
    list << p->ui->selected->item(row)->data(Qt::UserRole).toString();

  return list;
}

void
SideBySideMultiSelect::moveSelectedListWidgetItems(QListWidget &from,
                                                   QListWidget &to) {
  for (auto const &item : from.selectedItems()) {
    auto actualItem = from.takeItem(from.row(item));
    if (actualItem)
      to.addItem(actualItem);
  }

  emit listsChanged();
}

void
SideBySideMultiSelect::availableSelectionChanged() {
  auto p = p_func();

  auto hasSelected = !p->ui->available->selectedItems().isEmpty();
  p->ui->moveToSelectedButton->setEnabled(hasSelected);
}

void
SideBySideMultiSelect::selectedSelectionChanged() {
  auto p = p_func();

  auto hasSelected = !p->ui->selected->selectedItems().isEmpty();
  p->ui->moveToAvailableButton->setEnabled(hasSelected);
}

void
SideBySideMultiSelect::moveToAvailable() {
  auto p = p_func();

  moveSelectedListWidgetItems(*p->ui->selected, *p->ui->available);
}

void
SideBySideMultiSelect::moveToSelected() {
  auto p = p_func();

  moveSelectedListWidgetItems(*p->ui->available, *p->ui->selected);
}

void
SideBySideMultiSelect::revertLists() {
  auto p = p_func();

  setItems(p->initiallyAvailableItems, p->initiallySelectedItems);
}

}}}
