#include "common/common_pch.h"

#include <QByteArray>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QRegularExpression>
#include <QUrl>

#include "common/mime.h"
#include "common/mm_proxy_io.h"
#include "common/mm_text_io.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/util/file.h"
#include "mkvtoolnix-gui/util/message_box.h"
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

QStringList
replaceDirectoriesByContainedFiles(QStringList const &namesToCheck) {
  QStringList fileNames;

  for (auto const &nameToCheck : namesToCheck) {
    auto info = QFileInfo{nameToCheck};
    if (!info.exists())
      continue;

    if (info.isFile()) {
      fileNames << nameToCheck;
      continue;
    }

    if (!info.isDir())
      continue;

    QStringList newFileNames;
    QDirIterator it{nameToCheck, QDirIterator::Subdirectories};

    while (it.hasNext()) {
      it.next();
      info = it.fileInfo();

      if (info.isFile())
        newFileNames << info.absoluteFilePath();
    }

    newFileNames.sort(Qt::CaseInsensitive);
    fileNames += newFileNames;
  }

  return fileNames;
}

QString
detectMIMEType(QString const &fileName) {
  auto mimeType = ::mtx::mime::guess_type_for_file(to_utf8(fileName));
  return Q(::mtx::mime::maybe_map_to_legacy_font_mime_type(mimeType, Util::Settings::get().m_useLegacyFontMIMETypes));
}

void
saveTextToFile(QString const &fileName,
               QStringList const &content) {
  saveTextToFile(fileName, content.join(Q("\n")) + Q("\n"));
}

void
saveTextToFile(QString const &fileName,
               QString const &content) {
  QFile out{fileName};

  if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    MessageBox::critical(MainWindow::get())
      ->title(QY("Saving failed"))
      .text(QY("Saving the file failed. Error message from the system: %1").arg(out.errorString()))
      .exec();

    return;
  }

  out.write(content.toUtf8());
  out.flush();
  out.close();

  auto &cfg = Util::Settings::get();
  cfg.m_lastOpenDir.setPath(QFileInfo{fileName}.path());
  cfg.save();
}

}
