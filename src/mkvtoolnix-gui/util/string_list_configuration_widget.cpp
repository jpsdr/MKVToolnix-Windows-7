#include "common/common_pch.h"

#include <QInputDialog>
#include <QListWidget>
#include <QPushButton>
#include <QRegularExpression>

#include "common/qt.h"
#include "mkvtoolnix-gui/forms/util/string_list_configuration_widget.h"
#include "mkvtoolnix-gui/util/string_list_configuration_widget.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx { namespace gui { namespace Util {

namespace {

class Item: public QListWidgetItem {
public:
  std::pair<uint64_t, QString> m_values;

public:
  explicit Item(QString const &name);

  virtual bool operator <(QListWidgetItem const &cmp) const;
  void extractSortValues();
};

Item::Item(QString const &name)
  : QListWidgetItem{name}
{
  extractSortValues();
  setFlags(flags() | Qt::ItemIsEditable);
}

void
Item::extractSortValues() {
  auto name = text();

  QRegularExpressionMatch match;

  if (name.contains(QRegularExpression{Q("^([0-9]+)(.*)")}, &match)) {
    m_values.first  = match.captured(1).toULongLong();
    m_values.second = match.captured(2);

  } else {
    m_values.first  = std::numeric_limits<uint64_t>::max();
    m_values.second = name;
  }
}

bool
Item::operator <(QListWidgetItem const &cmp)
  const {
  auto &otherItem = static_cast<Item const &>(cmp);

  return m_values.first < otherItem.m_values.first ? true
       : m_values.first > otherItem.m_values.first ? false
       :                                             m_values.second.compare(otherItem.m_values.second, Qt::CaseInsensitive) < 0;
}

}

// ------------------------------------------------------------

class StringListConfigurationWidgetPrivate {
  friend class StringListConfigurationWidget;

  std::unique_ptr<Ui::StringListConfigurationWidget> ui{new Ui::StringListConfigurationWidget};
  QString addItemDialogTitle, addItemDialogText;
};

StringListConfigurationWidget::StringListConfigurationWidget(QWidget *parent)
  : QWidget{parent}
  , p_ptr{new StringListConfigurationWidgetPrivate}
{
  auto p = p_func();

  p->ui->setupUi(this);

  p->ui->pbRemoveItem->setEnabled(false);

  setupConnections();
}

StringListConfigurationWidget::~StringListConfigurationWidget() {
}

void
StringListConfigurationWidget::retranslateUi() {
  p_func()->ui->retranslateUi(this);
}

void
StringListConfigurationWidget::setToolTips(QString const &items,
                                           QString const &add,
                                           QString const &remove) {
  auto p = p_func();

  Util::setToolTip(p->ui->lwItems,      items);
  Util::setToolTip(p->ui->pbAddItem,    add);
  Util::setToolTip(p->ui->pbRemoveItem, remove);
}

void
StringListConfigurationWidget::setAddItemDialogTexts(QString const &title,
                                                     QString const &text) {
  auto p = p_func();

  p->addItemDialogTitle = title;
  p->addItemDialogText  = text;
}

void
StringListConfigurationWidget::setupConnections() {
  auto p = p_func();

  connect(p->ui->lwItems,      &QListWidget::itemSelectionChanged, this, &StringListConfigurationWidget::enableControls);
  connect(p->ui->lwItems,      &QListWidget::itemChanged,          this, &StringListConfigurationWidget::sortAfterItemChanged);
  connect(p->ui->pbAddItem,    &QPushButton::clicked,              this, &StringListConfigurationWidget::addNewItem);
  connect(p->ui->pbRemoveItem, &QPushButton::clicked,              this, &StringListConfigurationWidget::removeSelectedItems);
}

void
StringListConfigurationWidget::enableControls() {
  auto p = p_func();

  p->ui->pbRemoveItem->setEnabled(!p->ui->lwItems->selectedItems().isEmpty());
}

void
StringListConfigurationWidget::addNewItem() {
  auto p = p_func();

  auto newName = QInputDialog::getText(this, p->addItemDialogTitle, p->addItemDialogText);
  if (newName.isEmpty())
    return;

  addItem(newName);
}

void
StringListConfigurationWidget::addItem(QString const &name) {
  p_func()->ui->lwItems->addItem(new Item{name});
}

void
StringListConfigurationWidget::removeSelectedItems() {
  for (auto const &item : p_func()->ui->lwItems->selectedItems())
    delete item;
}

void
StringListConfigurationWidget::setItems(QStringList const &items) {
  auto p = p_func();

  p->ui->lwItems->clear();
  for (auto const &item : items)
    addItem(item);
}

QStringList
StringListConfigurationWidget::items()
  const {
  auto p    = p_func();
  auto list = QStringList{};

  for (auto row = 0, numRows = p->ui->lwItems->count(); row < numRows; ++row) {
    auto name = p->ui->lwItems->item(row)->text();
    if (!name.isEmpty())
      list << name;
  }

  return list;
}

void
StringListConfigurationWidget::sortAfterItemChanged(QListWidgetItem *item) {
  auto cfgItem = dynamic_cast<Item *>(item);
  if (!cfgItem)
    return;

  cfgItem->extractSortValues();
  p_func()->ui->lwItems->sortItems();
}

}}}
