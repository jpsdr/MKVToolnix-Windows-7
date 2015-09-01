#ifndef MTX_MKVTOOLNIX_GUI_UTIL_HEADER_VIEW_MANAGER_H
#define MTX_MKVTOOLNIX_GUI_UTIL_HEADER_VIEW_MANAGER_H

#include "common/common_pch.h"

#include <QObject>

class QString;
class QTreeView;

namespace mtx { namespace gui { namespace Util {

class HeaderViewManagerPrivate;
class HeaderViewManager : public QObject {
  Q_OBJECT;

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
  void restoreHidden(QStringList hiddenFlags);
  void restoreVisualIndexes(QStringList visualIndexes);

  Q_DECLARE_PRIVATE(HeaderViewManager);

  QScopedPointer<HeaderViewManagerPrivate> const d_ptr;
};

}}}

#endif // MTX_MKVTOOLNIX_GUI_UTIL_HEADER_VIEW_MANAGER_H
