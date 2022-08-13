#include "common/common_pch.h"

#include <QByteArray>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QRegularExpression>
#include <QUrl>

#include "common/mime.h"
#include "common/mm_proxy_io.h"
#include "common/mm_text_io.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/util/file.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/string.h"

namespace mtx::gui::Util {

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

  mm_text_io_c::detect_byte_order_marker(dataPtr, dataSize, result.byteOrderMark, result.bomLength);

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

QString
removeInvalidPathCharacters(QString fileName) {
#if defined(SYS_WINDOWS)
  static DeferredRegularExpression s_driveRE{Q("^[A-Za-z]:")}, s_invalidCharRE{Q("[<>\"|?*\\x{01}-\\x{1f}]+")}, s_invalidCharRE2{Q("[:]+")};

  fileName.remove(*s_invalidCharRE);

  QString drive;
  if (fileName.contains(*s_driveRE)) {
    drive = fileName.left(2);
    fileName.remove(0, 2);
  }

  return drive + fileName.remove(*s_invalidCharRE2);

#else
  return fileName;
#endif
}

QString
replaceInvalidFileNameCharacters(QString fileName) {
#if defined(SYS_WINDOWS)
  static DeferredRegularExpression s_invalidCharRE{Q(R"([\\/:<>"|?*\x{01}-\x{1f}]+)")};
#else
  static DeferredRegularExpression s_invalidCharRE{Q("/+")};
#endif

  fileName.replace(*s_invalidCharRE, Q("-"));

  return fileName;
}

QString
detectMIMEType(QString const &fileName) {
  auto mimeType = ::mtx::mime::guess_type_for_file(to_utf8(fileName));
  return Q(::mtx::mime::maybe_map_to_legacy_font_mime_type(mimeType, Util::Settings::get().m_useLegacyFontMIMETypes));
}

}
