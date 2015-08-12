#include "common/common_pch.h"

#include <QDragEnterEvent>
#include <QDragMoveEvent>

#include "common/list_utils.h"
#include "mkvtoolnix-gui/util/basic_line_edit.h"

namespace mtx { namespace gui { namespace Util {

using namespace mtx::gui;

class BasicLineEditPrivate {
  friend class BasicLineEdit;

  bool m_acceptDroppedFiles{true}, m_setTextToDroppedFileName{true};
  FilesDragDropHandler m_filesDDHandler{FilesDragDropHandler::Mode::Remember};
};

BasicLineEdit::BasicLineEdit(QWidget *parent)
  : QLineEdit{parent}
  , d_ptr{new BasicLineEditPrivate}
{
}

BasicLineEdit::~BasicLineEdit() {
}

BasicLineEdit &
BasicLineEdit::acceptDroppedFiles(bool enable) {
  Q_D(BasicLineEdit);

  d->m_acceptDroppedFiles = enable;
  return *this;
}

BasicLineEdit &
BasicLineEdit::setTextToDroppedFileName(bool enable) {
  Q_D(BasicLineEdit);

  d->m_setTextToDroppedFileName = enable;
  return *this;
}

void
BasicLineEdit::dragEnterEvent(QDragEnterEvent *event) {
  Q_D(BasicLineEdit);

  if (d->m_acceptDroppedFiles && d->m_filesDDHandler.handle(event, false))
    return;

  QLineEdit::dragEnterEvent(event);
}

void
BasicLineEdit::dragMoveEvent(QDragMoveEvent *event) {
  Q_D(BasicLineEdit);

  if (d->m_acceptDroppedFiles && d->m_filesDDHandler.handle(event, false))
    return;

  QLineEdit::dragMoveEvent(event);
}

void
BasicLineEdit::dropEvent(QDropEvent *event) {
  Q_D(BasicLineEdit);

  if (d->m_acceptDroppedFiles && d->m_filesDDHandler.handle(event, true)) {
    auto fileNames = d->m_filesDDHandler.fileNames();

    if (d->m_setTextToDroppedFileName && !fileNames.isEmpty())
      setText(fileNames[0]);

    emit filesDropped(fileNames);

    return;
  }

  QLineEdit::dropEvent(event);
}

}}}
