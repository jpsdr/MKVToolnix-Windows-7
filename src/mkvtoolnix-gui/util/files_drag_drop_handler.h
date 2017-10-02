#pragma once

#include "common/common_pch.h"

#include <QObject>
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

  bool handle(QDropEvent *event, bool isDrop);
  QStringList const & fileNames() const;
};

}}}
