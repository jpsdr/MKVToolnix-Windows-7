#include "common/common_pch.h"

#include <QDragEnterEvent>
#include <QDragMoveEvent>

#include "common/list_utils.h"
#include "mkvtoolnix-gui/util/basic_line_edit.h"

namespace mtx::gui::Util {

using namespace mtx::gui;

class BasicLineEditPrivate {
  friend class BasicLineEdit;

  bool m_acceptDroppedFiles{true}, m_setTextToDroppedFileName{true};
  FilesDragDropHandler m_filesDDHandler{FilesDragDropHandler::Mode::Remember};
};

BasicLineEdit::BasicLineEdit(QWidget *parent)
  : QLineEdit{parent}
  , p_ptr{new BasicLineEditPrivate}
{
}

BasicLineEdit::~BasicLineEdit() {
}

BasicLineEdit &
BasicLineEdit::acceptDroppedFiles(bool enable) {
  p_func()->m_acceptDroppedFiles = enable;
  return *this;
}

BasicLineEdit &
BasicLineEdit::setTextToDroppedFileName(bool enable) {
  p_func()->m_setTextToDroppedFileName = enable;
  return *this;
}

void
BasicLineEdit::dragEnterEvent(QDragEnterEvent *event) {
  auto p = p_func();

  if (p->m_acceptDroppedFiles && p->m_filesDDHandler.handle(event, false))
    return;

  QLineEdit::dragEnterEvent(event);
}

void
BasicLineEdit::dragMoveEvent(QDragMoveEvent *event) {
  auto p = p_func();

  if (p->m_acceptDroppedFiles && p->m_filesDDHandler.handle(event, false))
    return;

  QLineEdit::dragMoveEvent(event);
}

void
BasicLineEdit::dropEvent(QDropEvent *event) {
  auto p = p_func();

  if (p->m_acceptDroppedFiles && p->m_filesDDHandler.handle(event, true)) {
    auto fileNames = p->m_filesDDHandler.fileNames();

    if (p->m_setTextToDroppedFileName && !fileNames.isEmpty())
      setText(fileNames[0]);

    emit filesDropped(fileNames);

    return;
  }

  QLineEdit::dropEvent(event);
}

void
BasicLineEdit::keyPressEvent(QKeyEvent *event) {
  if (   (event->modifiers() == Qt::ShiftModifier)
      && mtx::included_in(static_cast<Qt::Key>(event->key()), Qt::Key_Return, Qt::Key_Enter)) {
    emit shiftReturnPressed();
    event->accept();

  } else
    QLineEdit::keyPressEvent(event);
}

}
