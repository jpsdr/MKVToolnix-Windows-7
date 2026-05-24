#include "common/common_pch.h"

#include <QDir>
#include <QStandardPaths>
#include <QString>

#include "common/path.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/util/file_dialog.h"
#include "mkvtoolnix-gui/util/message_box.h"

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
  auto oldPath = boost::filesystem::absolute(mtx::fs::to_path(dir));
  auto newPath = oldPath;
  auto ec      = boost::system::error_code{};

  while (   !boost::filesystem::is_directory(newPath, ec)
         && !newPath.parent_path().empty()
         && (newPath.parent_path() != newPath))
    newPath = newPath.parent_path();

  if (withFileName && (oldPath.filename() != "."))
    newPath /= oldPath.filename();

  // if (withFileName && (oldPath != newPath) && (oldPath.filename() != "."))
  //   newPath /= oldPath.filename();

  return Q(newPath.string());
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
                QFileDialog::Options options,
                QFileDialog::FileMode fileMode) {
  auto defaultName = sanitizeDirectory(dir, true);
  if (!defaultFileName.isEmpty())
    defaultName = QDir::toNativeSeparators(Q(mtx::fs::to_path(dir) / mtx::fs::to_path(defaultFileName)));

  while (true) {
    QFileDialog dlg{parent, caption, defaultName, filter};

    dlg.setDefaultSuffix(defaultSuffix);
    dlg.setOptions(options & QFileDialog::DontUseCustomDirectoryIcons);
    dlg.setFileMode(fileMode);
    dlg.setSupportedSchemes({ Q("file") });
    dlg.setAcceptMode(QFileDialog::AcceptSave);

    if (selectedFilter && !selectedFilter->isEmpty())
      dlg.selectNameFilter(*selectedFilter);

    if (dlg.exec() != QDialog::Accepted)
      return {};

    if (selectedFilter)
      *selectedFilter = dlg.selectedNameFilter();

    auto result = QDir::toNativeSeparators(dlg.selectedFiles().value(0));

    if (result.isEmpty())
      return result;

    if (!defaultSuffix.isEmpty() && QFileInfo{result}.suffix().isEmpty())
      result += Q(".") + defaultSuffix;

    if ((fileMode != QFileDialog::ExistingFile) || QFile{result}.exists())
      return result;

    MessageBox::critical(parent)
      ->title(QY("Select existing file"))
      .text(QY("You must select an existing file."))
      .exec();
  }

  return {};
}

QString
getExistingDirectory(QWidget *parent,
                     QString const &caption,
                     QString const &dir,
                     QFileDialog::Options options) {
  return QDir::toNativeSeparators(QFileDialog::getExistingDirectory(parent, caption, sanitizeDirectory(dir, false), options & QFileDialog::DontUseCustomDirectoryIcons));
}

}
