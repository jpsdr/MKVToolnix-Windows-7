#include "common/common_pch.h"

#include <QDir>
#include <QStandardPaths>
#include <QString>

#include "common/path.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/util/file_dialog.h"

namespace mtx::gui::Util {

QString
dirPath(QDir const &dir) {
  return dirPath(dir.path());
}

QString
dirPath(QString const &dir) {
  auto path = dir;

  if (path.isEmpty() || (path == Q(".")))
    path = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);

  if (path.isEmpty() || (path == Q(".")))
    path = QDir::currentPath();

  if (!QDir::toNativeSeparators(path).endsWith(QDir::separator()))
    path += Q("/");

  return QDir::fromNativeSeparators(path);
}

QString
sanitizeDirectory(QString const &directory,
                  bool withFileName) {
  auto dir     = to_utf8(directory.isEmpty() || (directory == Q(".")) ? QStandardPaths::writableLocation(QStandardPaths::MoviesLocation) : directory);
  auto oldPath = mtx::fs::absolute(mtx::fs::to_path(dir));
  auto newPath = oldPath;
  auto ec      = std::error_code{};

  while (   !std::filesystem::is_directory(newPath, ec)
         && !newPath.parent_path().empty()
         && (newPath.parent_path() != newPath))
    newPath = newPath.parent_path();

  if (withFileName && (oldPath.filename() != "."))
    newPath /= oldPath.filename();

  // if (withFileName && (oldPath != newPath) && (oldPath.filename() != "."))
  //   newPath /= oldPath.filename();

  return Q(newPath.u8string());
}

QString
getOpenFileName(QWidget *parent,
                QString const &caption,
                QString const &dir,
                QString const &filter,
                QString *selectedFilter,
                QFileDialog::Options options) {
  return QDir::toNativeSeparators(QFileDialog::getOpenFileName(parent, caption, sanitizeDirectory(dir, false), filter, selectedFilter, options & QFileDialog::DontUseCustomDirectoryIcons));
}

QStringList
getOpenFileNames(QWidget *parent,
                 QString const &caption,
                 QString const &dir,
                 QString const &filter,
                 QString *selectedFilter,
                 QFileDialog::Options options) {
  auto fileNames = QFileDialog::getOpenFileNames(parent, caption, sanitizeDirectory(dir, false), filter, selectedFilter, options & QFileDialog::DontUseCustomDirectoryIcons);
  for (auto &fileName : fileNames)
    fileName = QDir::toNativeSeparators(fileName);

  return fileNames;
}

QString
getSaveFileName(QWidget *parent,
                QString const &caption,
                QString const &dir,
                QString const &defaultFileName,
                QString const &filter,
                QString const &defaultSuffix,
                QString *selectedFilter,
                QFileDialog::Options options) {
  auto defaultName = sanitizeDirectory(dir, true);
  if (!defaultFileName.isEmpty())
    defaultName = QDir::toNativeSeparators(Q(mtx::fs::to_path(dir) / mtx::fs::to_path(defaultFileName)));

  auto result = QDir::toNativeSeparators(QFileDialog::getSaveFileName(parent, caption, defaultName, filter, selectedFilter, options & QFileDialog::DontUseCustomDirectoryIcons));

  if (result.isEmpty())
    return result;

  if (!defaultSuffix.isEmpty() && QFileInfo{result}.suffix().isEmpty())
    result += Q(".") + defaultSuffix;

  return result;
}

QString
getExistingDirectory(QWidget *parent,
                     QString const &caption,
                     QString const &dir,
                     QFileDialog::Options options) {
  return QDir::toNativeSeparators(QFileDialog::getExistingDirectory(parent, caption, sanitizeDirectory(dir, false), options & QFileDialog::DontUseCustomDirectoryIcons));
}

}
