#pragma once

#include "common/common_pch.h"

#include <QObject>

#include "common/qt.h"

class QString;
class QTreeView;

namespace mtx { namespace gui { namespace Util {

class HeaderViewManagerPrivate;
class HeaderViewManager : public QObject {
  Q_OBJECT;

protected:
  MTX_DECLARE_PRIVATE(HeaderViewManagerPrivate);

  std::unique_ptr<HeaderViewManagerPrivate> const p_ptr;

  explicit HeaderViewManager(HeaderViewManagerPrivate &p);

public:
  explicit HeaderViewManager(QObject *parent = nullptr);
  virtual ~HeaderViewManager();

  void manage(QTreeView &treeView, QString const &name);

public slots:
  void saveState();
  void restoreState();

  void showContextMenu(QPoint const &pos);
  void toggleColumn(int column);
  void resetColumns();

public:
  static HeaderViewManager *create(QTreeView &treeView, QString const &name);

protected:
  void restoreHidden(QStringList const &hiddenColumns);
  void restoreVisualIndexes(QStringList const &columnOrder);
  void restoreSizes(QStringList const &columnSizes);
  QString symbolicColumnName(int logicalIndex);
};

}}}
