#include "common/common_pch.h"

#include <QDir>
#include <QFile>
#include <QString>
#include <QStringList>
#include <QTemporaryFile>

#include "common/qt.h"
#include "mkvtoolnix-gui/util/option_file.h"
#include "mkvtoolnix-gui/util/util.h"

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
  auto file = std::unique_ptr<QTemporaryFile>(new QTemporaryFile{QDir::temp().filePath(prefix)});
  Q_ASSERT(file->open());
  write(*file, options);
  file->close();

  return file;
}

void
OptionFile::write(QFile &file,
                  QStringList const &options) {
  auto escapedOptions = Util::escape(options, Util::EscapeMkvtoolnix);

  static const unsigned char utf8_bom[3] = {0xef, 0xbb, 0xbf};

  file.write(reinterpret_cast<char const *>(utf8_bom), 3);
  for (auto &option : escapedOptions)
    file.write(Q("%1\n").arg(option).toUtf8());
}
