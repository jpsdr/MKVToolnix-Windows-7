#include "common/common_pch.h"

#include <QDir>
#include <QFile>
#include <QString>
#include <QStringList>
#include <QTemporaryFile>

#include "common/qt.h"
#include "mkvtoolnix-gui/util/file.h"
#include "mkvtoolnix-gui/util/option_file.h"
#include "mkvtoolnix-gui/util/process.h"
#include "mkvtoolnix-gui/util/string.h"

namespace mtx::gui::Util {

void
OptionFile::create(QString const &fileName,
                   QStringList const &options) {
  Util::saveTextToFile(fileName, escape(options, EscapeJSON)[0]);
}

std::unique_ptr<QTemporaryFile>
OptionFile::createTemporary(QString const &prefix,
                            QStringList const &options) {
  auto fileNameTemplate = QDir::temp().filePath(Q("%1-XXXXXX.json").arg(prefix));
  auto file             = std::make_unique<QTemporaryFile>(fileNameTemplate);

  if (!file->open()) {
    auto fileName = file->fileName().isEmpty() ? fileNameTemplate : file->fileName();
    throw ProcessX{ to_utf8(QY("Saving the file '%1' failed. Error message from the system: %2").arg(fileName).arg(file->errorString())) };
  }

  auto serialized = to_utf8(escape(options, EscapeJSON)[0]);

  file->write(serialized.c_str());
  file->close();

  return file;
}

}
