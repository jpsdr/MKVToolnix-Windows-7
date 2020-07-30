#pragma once

#include "common/common_pch.h"

#include <QList>
#include <QStringList>
#include <QWidget>

class QListWidget;
class QListWidgetItem;

namespace mtx::gui::Util {

namespace Ui {
class StringListConfigurationWidget;
}

class StringListConfigurationWidgetPrivate;
class StringListConfigurationWidget : public QWidget {
  Q_OBJECT

protected:
  MTX_DECLARE_PRIVATE(StringListConfigurationWidgetPrivate)

  std::unique_ptr<StringListConfigurationWidgetPrivate> const p_ptr;

  explicit StringListConfigurationWidget(StringListConfigurationWidgetPrivate &p);

public:
  explicit StringListConfigurationWidget(QWidget *parent = nullptr);
  ~StringListConfigurationWidget();

  void retranslateUi();
  void setToolTips(QString const &items, QString const &add = {}, QString const &remove = {});
  void setAddItemDialogTexts(QString const &title, QString const &text);
  void setItems(QStringList const &items);

  QStringList items() const;

  void addItem(QString const &name);

public Q_SLOTS:
  void enableControls();

  void addNewItem();
  void removeSelectedItems();

protected:
  void setupConnections();
};

}
