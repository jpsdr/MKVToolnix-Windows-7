#include "common/common_pch.h"

#include <QDir>
#include <QDropEvent>
#include <QMimeData>

#include "mkvtoolnix-gui/util/files_drag_drop_handler.h"

namespace mtx::gui::Util {

class FilesDragDropHandlerPrivate {
private:
  friend class FilesDragDropHandler;

  QStringList m_fileNames;
  FilesDragDropHandler::Mode m_mode{FilesDragDropHandler::Mode::PassThrough};

  FilesDragDropHandlerPrivate(FilesDragDropHandler::Mode mode)
    : m_mode{mode}
  {
  }
};

FilesDragDropHandler::FilesDragDropHandler(Mode mode)
  : QObject{}
  , p_ptr{new FilesDragDropHandlerPrivate{mode}}
{
}

FilesDragDropHandler::FilesDragDropHandler(FilesDragDropHandlerPrivate &p)
  : QObject{}
  , p_ptr{&p}
{
}

FilesDragDropHandler::~FilesDragDropHandler() {
}

bool
FilesDragDropHandler::handle(QDropEvent *event,
                             bool isDrop) {
  auto p = p_func();

  event->ignore();

  if (!event->mimeData()->hasUrls())
    return false;

  if (isDrop && (Mode::Remember == p->m_mode))
    p->m_fileNames = QStringList{};

  for (auto const &url : event->mimeData()->urls())
    if (!url.isLocalFile())
      return false;
    else if (isDrop && (Mode::Remember == p->m_mode))
      p->m_fileNames << QDir::toNativeSeparators(url.toLocalFile());

  if (Mode::Remember == p->m_mode) {
    event->acceptProposedAction();
    event->setDropAction(Qt::CopyAction);
  }

  return true;
}

QStringList const &
FilesDragDropHandler::fileNames()
  const {
  return p_func()->m_fileNames;
}

}
