#include "common/common_pch.h"

#include <QDir>
#include <QInputDialog>
#include <QKeyEvent>
#include <QKeySequence>
#include <QListWidget>
#include <QPushButton>
#include <QRegularExpression>

#include "common/qt.h"
#include "mkvtoolnix-gui/forms/util/string_list_configuration_widget.h"
#include "mkvtoolnix-gui/util/file_dialog.h"
#include "mkvtoolnix-gui/util/string_list_configuration_widget.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx::gui::Util {

// ------------------------------------------------------------

class StringListConfigurationWidgetPrivate {
  friend class StringListConfigurationWidget;

  std::unique_ptr<Ui::StringListConfigurationWidget> ui{new Ui::StringListConfigurationWidget};
  QString addItemDialogTitle, addItemDialogText;
  StringListConfigurationWidget::ItemType itemType{StringListConfigurationWidget::ItemType::String};
  std::optional<unsigned int> maximumNumItems;
};

StringListConfigurationWidget::StringListConfigurationWidget(QWidget *parent)
  : QWidget{parent}
  , p_ptr{new StringListConfigurationWidgetPrivate}
{
  auto p = p_func();

  p->ui->setupUi(this);

  p->ui->pbRemoveItem->setEnabled(false);

  p->ui->lwItems->installEventFilter(this);

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
StringListConfigurationWidget::setMaximumNumItems(unsigned int maximumNumItems) {
  p_func()->maximumNumItems = maximumNumItems;
  enableControls();
}

void
StringListConfigurationWidget::setupConnections() {
  auto p = p_func();

  connect(p->ui->lwItems,      &QListWidget::itemSelectionChanged, this, &StringListConfigurationWidget::enableControls);
  connect(p->ui->pbAddItem,    &QPushButton::clicked,              this, &StringListConfigurationWidget::addNewItem);
  connect(p->ui->pbRemoveItem, &QPushButton::clicked,              this, &StringListConfigurationWidget::removeSelectedItems);
}

void
StringListConfigurationWidget::enableControls() {
  auto p = p_func();

  p->ui->pbAddItem->setEnabled(!p->maximumNumItems || (static_cast<unsigned int>(p->ui->lwItems->count()) < *p->maximumNumItems));
  p->ui->pbRemoveItem->setEnabled(!p->ui->lwItems->selectedItems().isEmpty());
}

void
StringListConfigurationWidget::addNewItem() {
  auto p = p_func();

  QString newValue;

  if (p->itemType == ItemType::String)
    newValue = QInputDialog::getText(this, p->addItemDialogTitle, p->addItemDialogText);

  else {
    newValue = Util::getExistingDirectory(this, p->addItemDialogTitle);

    for (auto row = 0, numRows = p->ui->lwItems->count(); row < numRows; ++row) {
      auto value = p->ui->lwItems->item(row)->text();
      if (value == newValue)
        return;
    }
  }

  if (newValue.isEmpty())
    return;

  addItem(newValue);
  enableControls();
}

void
StringListConfigurationWidget::addItem(QString const &name) {
  auto item = new QListWidgetItem{name};
  item->setFlags(item->flags() | Qt::ItemIsEditable);

  p_func()->ui->lwItems->addItem(item);
}

void
StringListConfigurationWidget::removeSelectedItems() {
  for (auto const &item : p_func()->ui->lwItems->selectedItems())
    delete item;

  enableControls();
}

void
StringListConfigurationWidget::setItems(QStringList const &items) {
  auto p                 = p_func();
  auto const isDirectory = p->itemType == ItemType::Directory;

  p->ui->lwItems->clear();
  for (auto const &item : items)
    addItem(isDirectory ? QDir::toNativeSeparators(item) : item);
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
StringListConfigurationWidget::setItemType(ItemType itemType) {
  p_func()->itemType = itemType;
}

bool
StringListConfigurationWidget::eventFilter(QObject *o,
                                           QEvent *e) {
  auto p = p_func();

  if (   (o           == p->ui->lwItems)
      && (e->type()   == QEvent::KeyPress)
      && (static_cast<QKeyEvent *>(e)->matches(QKeySequence::Delete))) {
    removeSelectedItems();
    return true;
  }

  return QWidget::eventFilter(o, e);
}

}
