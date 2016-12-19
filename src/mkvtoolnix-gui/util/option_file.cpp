#include "common/common_pch.h"

#include <QDir>
#include <QFile>
#include <QString>
#include <QStringList>
#include <QTemporaryFile>

#include "common/qt.h"
#include "mkvtoolnix-gui/util/option_file.h"
#include "mkvtoolnix-gui/util/process.h"
#include "mkvtoolnix-gui/util/string.h"

namespace mtx { namespace gui { namespace Util {

void
OptionFile::create(QString const &fileName,
                   QStringList const &options) {
  QFile file{fileName};
  file.open(QIODevice::WriteOnly | QIODevice::Truncate);
  write(file, options);
  file.close();
}

std::unique_ptr<QTemporaryFile>
OptionFile::createTemporary(QString const &prefix,
                            QStringList const &options) {
  auto file = std::make_unique<QTemporaryFile>(QDir::temp().filePath(Q("%1-XXXXXX.json").arg(prefix)));

  if (!file->open())
    throw ProcessX{ to_utf8(QY("Error creating a temporary file (reason: %1).").arg(file->errorString())) };

  write(*file, options);

  file->close();

  return file;
}

void
OptionFile::write(QFile &file,
                  QStringList const &options) {
  auto serialized = to_utf8(escape(options, EscapeJSON)[0]);

  file.write(serialized.c_str());
}

}}}
