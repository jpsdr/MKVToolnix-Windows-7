#include "common/common_pch.h"

#include <QDragEnterEvent>
#include <QDropEvent>

#include "mkvtoolnix-gui/util/files_drag_drop_widget.h"

namespace mtx::gui::Util {

class FilesDragDropWidgetPrivate {
private:
  friend class FilesDragDropWidget;

  mtx::gui::Util::FilesDragDropHandler m_filesDDHandler{Util::FilesDragDropHandler::Mode::Remember};
};

FilesDragDropWidget::FilesDragDropWidget(QWidget *parent)
  : QWidget{parent}
  , p_ptr{new FilesDragDropWidgetPrivate}
{
}

FilesDragDropWidget::FilesDragDropWidget(QWidget *parent,
                                         FilesDragDropWidgetPrivate &p)
  : QWidget{parent}
  , p_ptr{&p}
{
}

FilesDragDropWidget::~FilesDragDropWidget() {
}

void
FilesDragDropWidget::dragEnterEvent(QDragEnterEvent *event) {
  p_func()->m_filesDDHandler.handle(event, false);
}

void
FilesDragDropWidget::dropEvent(QDropEvent *event) {
  auto p = p_func();

  if (p->m_filesDDHandler.handle(event, true))
    emit filesDropped(p->m_filesDDHandler.fileNames());
}

}
