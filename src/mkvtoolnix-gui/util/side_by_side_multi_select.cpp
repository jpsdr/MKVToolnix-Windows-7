#include "common/common_pch.h"

#include <QListWidget>
#include <QPushButton>

#include "common/qt.h"
#include "mkvtoolnix-gui/forms/util/side_by_side_multi_select.h"
#include "mkvtoolnix-gui/util/side_by_side_multi_select.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx { namespace gui { namespace Util {

SideBySideMultiSelect::SideBySideMultiSelect(QWidget *parent)
  : QWidget{parent}
  , ui{new Ui::SideBySideMultiSelect}
{
  ui->setupUi(this);

  ui->moveToAvailableButton->setEnabled(false);
  ui->moveToSelectedButton->setEnabled(false);

  setupConnections();
}

SideBySideMultiSelect::~SideBySideMultiSelect() {
}

void
SideBySideMultiSelect::retranslateUi() {
  ui->retranslateUi(this);
}

void
SideBySideMultiSelect::setToolTips(QString const &available,
                                   QString const &selected) {
  Util::setToolTip(ui->available, available);
  Util::setToolTip(ui->selected,  selected);
}

void
SideBySideMultiSelect::setupConnections() {
  connect(ui->available->selectionModel(), &QItemSelectionModel::selectionChanged, this, &SideBySideMultiSelect::availableSelectionChanged);
  connect(ui->selected->selectionModel(),  &QItemSelectionModel::selectionChanged, this, &SideBySideMultiSelect::selectedSelectionChanged);

  connect(ui->moveToAvailableButton,       &QPushButton::clicked,                  this, &SideBySideMultiSelect::moveToAvailable);
  connect(ui->moveToSelectedButton,        &QPushButton::clicked,                  this, &SideBySideMultiSelect::moveToSelected);
}

void
SideBySideMultiSelect::setItems(ItemList const &all,
                                QStringList const &selected) {
  ui->available->clear();
  ui->selected->clear();

  auto isSelected = QHash<QString, bool>{};

  for (auto const &item : selected)
    isSelected[item] = true;

  for (auto const &item : all) {
    auto addTo  = isSelected[item.second] ? ui->selected : ui->available;
    auto lwItem = new QListWidgetItem{item.first};

    lwItem->setData(Qt::UserRole, item.second);
    addTo->addItem(lwItem);
  }

  ui->moveToAvailableButton->setEnabled(false);
  ui->moveToSelectedButton->setEnabled(false);
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
  auto list = ItemList{};

  for (auto row = 0, numRows = ui->selected->count(); row < numRows; ++row) {
    auto item = ui->selected->item(row);
    list << std::make_pair(item->text(), item->data(Qt::UserRole).toString());
  }

  return list;
}

QStringList
SideBySideMultiSelect::selectedItemValues()
  const {
  auto list = QStringList{};

  for (auto row = 0, numRows = ui->selected->count(); row < numRows; ++row)
    list << ui->selected->item(row)->data(Qt::UserRole).toString();

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
  auto hasSelected = !ui->available->selectedItems().isEmpty();
  ui->moveToSelectedButton->setEnabled(hasSelected);
}

void
SideBySideMultiSelect::selectedSelectionChanged() {
  auto hasSelected = !ui->selected->selectedItems().isEmpty();
  ui->moveToAvailableButton->setEnabled(hasSelected);
}

void
SideBySideMultiSelect::moveToAvailable() {
  moveSelectedListWidgetItems(*ui->selected, *ui->available);
}

void
SideBySideMultiSelect::moveToSelected() {
  moveSelectedListWidgetItems(*ui->available, *ui->selected);
}

}}}
