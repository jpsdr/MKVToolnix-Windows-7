#pragma once

#include "common/common_pch.h"

#include <QString>
#include <QWidget>

class QStandardItem;

namespace mtx { namespace gui { namespace Info {

namespace Ui {
class Tab;
}

class Model;
class TabPrivate;
class Tab : public QWidget {
  Q_OBJECT;

protected:
  MTX_DECLARE_PRIVATE(TabPrivate);

  std::unique_ptr<TabPrivate> const p_ptr;

  explicit Tab(TabPrivate &p);

public:
  explicit Tab(QWidget *parent);
  ~Tab();

  QString title() const;
  void load(QString const &fileName);
  void save();

signals:
  void removeThisTab();
  void titleChanged();

public slots:
  void retranslateUi();
  void showError(const QString &message);
  void expandImportantElements();
  void readLevel1Element(QModelIndex const &idx);
  void showElementHexDumpInViewer();
  void showContextMenu(QPoint const &pos);

protected:
  void setItemsFromElement(QList<QStandardItem *> &items, EbmlElement &element);
};

}}}
