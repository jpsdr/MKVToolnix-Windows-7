#pragma once

#include "common/common_pch.h"

class QString;
class QUrl;

namespace mtx { namespace gui { namespace Util {

struct BomAsciiCheckResult {
  byte_order_e byteOrder{BO_NONE};
  unsigned int bomLength{};
  bool containsNonAscii{};
};

BomAsciiCheckResult checkForBomAndNonAscii(QString const &fileName);

QUrl pathToFileUrl(QString const &path);

QString removeInvalidPathCharacters(QString fileName);

QStringList replaceDirectoriesByContainedFiles(QStringList const &namesToCheck);

}}}
