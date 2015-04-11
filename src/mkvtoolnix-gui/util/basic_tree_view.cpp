#include "common/common_pch.h"

#include <QDragMoveEvent>

#include "mkvtoolnix-gui/util/basic_tree_view.h"

namespace mtx { namespace gui { namespace Util {

using namespace mtx::gui;

BasicTreeView::BasicTreeView(QWidget *parent)
  : QTreeView{parent}
{
}

BasicTreeView::~BasicTreeView() {
}

BasicTreeView &
BasicTreeView::dropInFirstColumnOnly(bool enable) {
  m_dropInFirstColumnOnly = enable;
  return *this;
}

void
BasicTreeView::dragMoveEvent(QDragMoveEvent *event) {
  if (m_dropInFirstColumnOnly && (event->pos().x() > columnWidth(0))) {
    event->ignore();
    return;
  }

  QTreeView::dragMoveEvent(event);
}

}}}
