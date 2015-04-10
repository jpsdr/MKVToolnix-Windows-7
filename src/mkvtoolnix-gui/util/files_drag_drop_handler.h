#ifndef MTX_MKVTOOLNIX_GUI_UTIL_FILES_DRAG_DROP_HANDLER_H
#define MTX_MKVTOOLNIX_GUI_UTIL_FILES_DRAG_DROP_HANDLER_H

#include "common/common_pch.h"

#include <QStringList>

class QDropEvent;

namespace mtx { namespace gui { namespace Util {

class FilesDragDropHandler: public QObject {
  Q_OBJECT;

public:
  enum class Mode {
    PassThrough,
    Remember,
  };

protected:
  QStringList m_fileNames;
  Mode m_mode;

public:
  FilesDragDropHandler(Mode mode);

  bool handle(QDropEvent *event);
  QStringList const & getFileNames() const;
};

}}}

#endif  // MTX_MKVTOOLNIX_GUI_UTIL_FILES_DRAG_DROP_HANDLER_H
