#ifndef MTX_MKVTOOLNIX_GUI_UTIL_SIDE_BY_SIDE_MULTI_SELECT_H
#define MTX_MKVTOOLNIX_GUI_UTIL_SIDE_BY_SIDE_MULTI_SELECT_H

#include "common/common_pch.h"

#include <QList>
#include <QStringList>
#include <QWidget>

class QListWidget;

namespace mtx { namespace gui { namespace Util {

namespace Ui {
class SideBySideMultiSelect;
}

class SideBySideMultiSelect : public QWidget {
  Q_OBJECT;

protected:
  // UI stuff:
  std::unique_ptr<Ui::SideBySideMultiSelect> ui;

public:
  using Item     = std::pair<QString, QString>;
  using ItemList = QList<Item>;

public:
  explicit SideBySideMultiSelect(QWidget *parent = nullptr);
  ~SideBySideMultiSelect();

  void retranslateUi();
  void setToolTips(QString const &available, QString const &selected);
  void setItems(ItemList const &all, QStringList const &selected);
  void setItems(QStringList const &all, QStringList const &selected);

  ItemList selectedItems() const;
  QStringList selectedItemValues() const;

signals:
  void listsChanged();

public slots:
  void availableSelectionChanged();
  void selectedSelectionChanged();

  void moveToAvailable();
  void moveToSelected();

protected:
  void setupConnections();

  void moveSelectedListWidgetItems(QListWidget &from, QListWidget &to);
};

}}}

#endif // MTX_MKVTOOLNIX_GUI_UTIL_SIDE_BY_SIDE_MULTI_SELECT_H
