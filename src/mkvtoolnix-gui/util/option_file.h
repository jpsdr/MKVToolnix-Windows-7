#ifndef MTX_MKVTOOLNIX_GUI_UTIL_OPTION_FILE_H
#define MTX_MKVTOOLNIX_GUI_UTIL_OPTION_FILE_H

#include "common/common_pch.h"

class QFile;
class QString;
class QStringList;
class QTemporaryFile;

class OptionFile {
public:
  static void create(QString const &fileName, QStringList const &options);
  static std::unique_ptr<QTemporaryFile> createTemporary(QString const &prefix, QStringList const &options);

  static void write(QFile &file, QStringList const &options);
};

#endif  // MTX_MKVTOOLNIX_GUI_UTIL_OPTION_FILE_H
