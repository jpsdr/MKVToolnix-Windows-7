#include "common/common_pch.h"

#include <QDragEnterEvent>
#include <QDragMoveEvent>

#include "mkvtoolnix-gui/util/basic_tree_view.h"

namespace mtx { namespace gui { namespace Util {

using namespace mtx::gui;

BasicTreeView::BasicTreeView(QWidget *parent)
  : QTreeView{parent}
  , m_filesDDHandler{FilesDragDropHandler::Mode::Remember}
{
}

BasicTreeView::~BasicTreeView() {
}

BasicTreeView &
BasicTreeView::acceptDroppedFiles(bool enable) {
  m_acceptDroppedFiles = enable;
  return *this;
}

BasicTreeView &
BasicTreeView::dropInFirstColumnOnly(bool enable) {
  m_dropInFirstColumnOnly = enable;
  return *this;
}

void
BasicTreeView::dragEnterEvent(QDragEnterEvent *event) {
  if (m_acceptDroppedFiles && m_filesDDHandler.handle(event, false))
    return;

  QTreeView::dragEnterEvent(event);
}

void
BasicTreeView::dragMoveEvent(QDragMoveEvent *event) {
  if (m_acceptDroppedFiles && m_filesDDHandler.handle(event, false))
    return;

  if (m_dropInFirstColumnOnly && (event->pos().x() > columnWidth(0))) {
    event->ignore();
    return;
  }

  QTreeView::dragMoveEvent(event);
}

void
BasicTreeView::dropEvent(QDropEvent *event) {
  if (m_acceptDroppedFiles && m_filesDDHandler.handle(event, true)) {
    emit filesDropped(m_filesDDHandler.fileNames());
    return;
  }

  QTreeView::dropEvent(event);
}

}}}
