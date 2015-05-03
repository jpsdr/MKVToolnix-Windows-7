#include "common/common_pch.h"

#include <QDragEnterEvent>
#include <QDropEvent>

#include "mkvtoolnix-gui/util/files_drag_drop_widget.h"

namespace mtx { namespace gui { namespace Util {

FilesDragDropWidget::FilesDragDropWidget(QWidget *parent)
  : QWidget{parent}
  , m_filesDDHandler{Util::FilesDragDropHandler::Mode::Remember}
{
}

FilesDragDropWidget::~FilesDragDropWidget() {
}

void
FilesDragDropWidget::dragEnterEvent(QDragEnterEvent *event) {
  m_filesDDHandler.handle(event, false);
}

void
FilesDragDropWidget::dropEvent(QDropEvent *event) {
  if (m_filesDDHandler.handle(event, true))
    emit filesDropped(m_filesDDHandler.fileNames());
}

}}}
