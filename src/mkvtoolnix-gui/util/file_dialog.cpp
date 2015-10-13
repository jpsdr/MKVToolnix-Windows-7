#include "common/common_pch.h"

#include <QStandardPaths>
#include <QString>

#include "common/qt.h"
#include "mkvtoolnix-gui/util/file_dialog.h"

namespace mtx { namespace gui { namespace Util {

QString
sanitizeDirectory(QString const &directory) {
  auto dir  = to_utf8(directory.isEmpty() || (directory == Q(".")) ? QStandardPaths::writableLocation(QStandardPaths::MoviesLocation) : directory);
  auto path = bfs::absolute(bfs::path{dir});
  auto ec   = boost::system::error_code{};

  while (   !(bfs::exists(path, ec) && bfs::is_directory(path, ec))
         && !path.parent_path().empty())
    path = path.parent_path();

  return Q(path.string());
}

QString
getOpenFileName(QWidget *parent,
                QString const &caption,
                QString const &dir,
                QString const &filter,
                QString *selectedFilter,
                QFileDialog::Options options) {
  return QFileDialog::getOpenFileName(parent, caption, sanitizeDirectory(dir), filter, selectedFilter, options & QFileDialog::DontUseCustomDirectoryIcons);
}

QStringList
getOpenFileNames(QWidget *parent,
                 QString const &caption,
                 QString const &dir,
                 QString const &filter,
                 QString *selectedFilter,
                 QFileDialog::Options options) {
  return QFileDialog::getOpenFileNames(parent, caption, sanitizeDirectory(dir), filter, selectedFilter, options & QFileDialog::DontUseCustomDirectoryIcons);
}

QString
getSaveFileName(QWidget *parent,
                QString const &caption,
                QString const &dir,
                QString const &filter,
                QString *selectedFilter,
                QFileDialog::Options options) {
  return QFileDialog::getSaveFileName(parent, caption, sanitizeDirectory(dir), filter, selectedFilter, options & QFileDialog::DontUseCustomDirectoryIcons);
}

QString
getExistingDirectory(QWidget *parent,
                     QString const &caption,
                     QString const &dir,
                     QFileDialog::Options options) {
  return QFileDialog::getExistingDirectory(parent, caption, sanitizeDirectory(dir), options & QFileDialog::DontUseCustomDirectoryIcons);
}

}}}
