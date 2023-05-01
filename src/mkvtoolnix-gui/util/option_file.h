#pragma once

#include "common/common_pch.h"

class QFile;
class QString;
class QTemporaryFile;

namespace mtx::gui::Util {

class OptionFile {
public:
  static void create(QString const &fileName, QStringList const &options);
  static std::unique_ptr<QTemporaryFile> createTemporary(QString const &prefix, QStringList const &options);
};

}
