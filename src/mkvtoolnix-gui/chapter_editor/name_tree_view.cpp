#include "common/common_pch.h"

#include <QDragMoveEvent>
#include <QDropEvent>

#include "mkvtoolnix-gui/chapter_editor/name_tree_view.h"

namespace mtx { namespace gui { namespace ChapterEditor {

using namespace mtx::gui;

NameTreeView::NameTreeView(QWidget *parent)
  : QTreeView{parent}
{
}

NameTreeView::~NameTreeView() {
}

void
NameTreeView::dragMoveEvent(QDragMoveEvent *event) {
  if (event->pos().x() > columnWidth(0)) {
    event->ignore();
    return;
  }

  QTreeView::dragMoveEvent(event);
}

void
NameTreeView::dropEvent(QDropEvent *event) {
  if (event->pos().x() > columnWidth(0)) {
    event->ignore();
    return;
  }

  QTreeView::dropEvent(event);
}

}}}
