#ifndef MTX_MKVTOOLNIX_GUI_UTIL_FILE_H
#define MTX_MKVTOOLNIX_GUI_UTIL_FILE_H

#include "common/common_pch.h"

class QString;

namespace mtx { namespace gui { namespace Util {

struct BomAsciiCheckResult {
  byte_order_e byteOrder{BO_NONE};
  unsigned int bomLength{};
  bool containsNonAscii{};
};

BomAsciiCheckResult checkForBomAndNonAscii(QString const &fileName);

}}}

#endif  // MTX_MKVTOOLNIX_GUI_UTIL_FILE_H
