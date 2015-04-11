#ifndef MTX_MKVTOOLNIX_GUI_UTIL_BASIC_TREE_VIEW_H
#define MTX_MKVTOOLNIX_GUI_UTIL_BASIC_TREE_VIEW_H

#include "common/common_pch.h"

#include <QTreeView>

class QDragMoveEvent;

namespace mtx { namespace gui { namespace Util {

class BasicTreeView : public QTreeView {
  Q_OBJECT;

protected:
  bool m_dropInFirstColumnOnly{true};

public:
  BasicTreeView(QWidget *parent);
  virtual ~BasicTreeView();

  BasicTreeView &dropInFirstColumnOnly(bool enable);

protected:
  virtual void dragMoveEvent(QDragMoveEvent *event) override;
};

}}}

#endif  // MTX_MKVTOOLNIX_GUI_UTIL_BASIC_TREE_VIEW_H
