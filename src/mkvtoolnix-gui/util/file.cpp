#include "common/common_pch.h"

#include <QByteArray>
#include <QFile>
#include <QIODevice>
#include <QUrl>

#include "common/qt.h"
#include "mkvtoolnix-gui/util/file.h"

namespace mtx { namespace gui { namespace Util {

BomAsciiCheckResult
checkForBomAndNonAscii(QString const &fileName) {
  QFile file{fileName};
  if (!file.open(QIODevice::ReadOnly))
    return {};

  auto content = file.readAll();
  file.close();

  auto result   = BomAsciiCheckResult{};
  auto dataSize = content.size();
  auto dataPtr  = reinterpret_cast<unsigned char const *>(content.constData());
  auto dataEnd  = dataPtr + dataSize;

  mm_text_io_c::detect_byte_order_marker(dataPtr, dataSize, result.byteOrder, result.bomLength);

  dataPtr += result.bomLength;

  while (dataPtr < dataEnd) {
    if (*dataPtr > 127) {
      result.containsNonAscii = true;
      break;
    }

    ++dataPtr;
  }

  return result;
}

QUrl
pathToFileUrl(QString const &path) {
  auto url = QUrl{};
  url.setScheme(Q("file"));
  url.setPath(path);
  return url;
}

}}}
