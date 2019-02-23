#pragma once

#include "common/common_pch.h"

#include <QObject>
#include <QStringList>

#include "common/qt.h"

class QDropEvent;

namespace mtx { namespace gui { namespace Util {

class FilesDragDropHandlerPrivate;
class FilesDragDropHandler: public QObject {
  Q_OBJECT

protected:
  MTX_DECLARE_PRIVATE(FilesDragDropHandlerPrivate)

  std::unique_ptr<FilesDragDropHandlerPrivate> const p_ptr;

  explicit FilesDragDropHandler(FilesDragDropHandlerPrivate &p);

public:
  enum class Mode {
    PassThrough,
    Remember,
  };

public:
  FilesDragDropHandler(Mode mode);
  virtual ~FilesDragDropHandler();

  bool handle(QDropEvent *event, bool isDrop);
  QStringList const & fileNames() const;
};

}}}
