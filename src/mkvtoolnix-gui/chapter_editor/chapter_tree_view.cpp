#include "common/common_pch.h"

#include <QDragEnterEvent>
#include <QDragMoveEvent>

#include "mkvtoolnix-gui/chapter_editor/chapter_tree_view.h"

namespace mtx { namespace gui { namespace ChapterEditor {

using namespace mtx::gui;

ChapterTreeView::ChapterTreeView(QWidget *parent)
  : QTreeView{parent}
  , m_fileDDHandler{Util::FilesDragDropHandler::Mode::PassThrough}
{
}

ChapterTreeView::~ChapterTreeView() {
}

void
ChapterTreeView::dragMoveEvent(QDragMoveEvent *event) {
  if (!m_fileDDHandler.handle(event) && (event->pos().x() <= columnWidth(0)))
    QTreeView::dragMoveEvent(event);
}

}}}
