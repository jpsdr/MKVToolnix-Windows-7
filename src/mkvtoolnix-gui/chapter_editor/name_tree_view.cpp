#include "common/common_pch.h"

#include <QDragMoveEvent>
#include <QDropEvent>

#include "mkvtoolnix-gui/chapter_editor/name_tree_view.h"

namespace mtx { namespace gui { namespace ChapterEditor {

using namespace mtx::gui;

NameTreeView::NameTreeView(QWidget *parent)
  : QTreeView{parent}
  , m_fileDDHandler{Util::FilesDragDropHandler::Mode::PassThrough}
{
}

NameTreeView::~NameTreeView() {
}

void
NameTreeView::dragMoveEvent(QDragMoveEvent *event) {
  if (!m_fileDDHandler.handle(event) && (event->pos().x() <= columnWidth(0)))
    QTreeView::dragMoveEvent(event);
}

}}}
