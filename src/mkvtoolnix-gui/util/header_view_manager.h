#ifndef MTX_MKVTOOLNIX_GUI_UTIL_HEADER_VIEW_MANAGER_H
#define MTX_MKVTOOLNIX_GUI_UTIL_HEADER_VIEW_MANAGER_H

#include "common/common_pch.h"

#include <QObject>

class QHeaderView;
class QString;

namespace mtx { namespace gui { namespace Util {

class HeaderViewManagerPrivate;
class HeaderViewManager : public QObject {
  Q_OBJECT;

public:
  explicit HeaderViewManager(QObject *parent = nullptr);
  virtual ~HeaderViewManager();

  void manage(QHeaderView &headerView, QString const &name);

public slots:
  void saveState();
  void restoreState();
  void showContextMenu(QPoint const &pos);
  void toggleColumn(int column);

public:
  static HeaderViewManager *create(QHeaderView &headerView, QString const &name);

protected:
  Q_DECLARE_PRIVATE(HeaderViewManager);

  QScopedPointer<HeaderViewManagerPrivate> const d_ptr;
};

}}}

#endif // MTX_MKVTOOLNIX_GUI_UTIL_HEADER_VIEW_MANAGER_H
