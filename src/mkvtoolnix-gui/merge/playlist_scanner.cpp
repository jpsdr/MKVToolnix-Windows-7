#include "common/common_pch.h"

#include "mkvtoolnix-gui/merge/ask_scan_for_playlists_dialog.h"
#include "mkvtoolnix-gui/merge/playlist_scanner.h"
#include "mkvtoolnix-gui/merge/select_playlist_dialog.h"
#include "mkvtoolnix-gui/util/file_identifier.h"
#include "mkvtoolnix-gui/util/message_box.h"
#include "mkvtoolnix-gui/util/settings.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QProgressDialog>
#include <QString>

namespace mtx { namespace gui { namespace Merge {

using namespace mtx::gui;

PlaylistScanner::PlaylistScanner(QWidget *parent)
  : m_parent{parent}
{
}

SourceFilePtr
PlaylistScanner::checkOneFile(SourceFilePtr const &file)
  const {
  if (!file->isPlaylist())
    return file;

  auto info     = QFileInfo{file->m_fileName};
  auto allFiles = QDir{info.path()}.entryInfoList(QStringList{QString{"*.%1"}.arg(info.suffix())}, QDir::Files, QDir::Name);
  if (allFiles.count() < 2)
    return file;

  auto doScan = Util::Settings::AlwaysScan == Util::Settings::get().m_scanForPlaylistsPolicy;
  if (!doScan)
    doScan = askScanForPlaylists(*file, allFiles.size() - 1);

  if (!doScan)
    return file;

  auto identifiedFiles = scanForPlaylists(allFiles);
  if (identifiedFiles.isEmpty())
    return file;

  if (1 == identifiedFiles.size())
    return identifiedFiles[0];

  return SelectPlaylistDialog{m_parent, identifiedFiles}.select();
}

QList<SourceFilePtr>
PlaylistScanner::checkAddingPlaylists(QList<SourceFilePtr> const &files)
  const {
  if (Util::Settings::NeverScan == Util::Settings::get().m_scanForPlaylistsPolicy)
    return files;

  auto actualFiles = QList<SourceFilePtr>{};

  for (auto &file : files) {
    auto actualFile = checkOneFile(file);
    if (actualFile)
      actualFiles << actualFile;
  }

  return actualFiles;
}

bool
PlaylistScanner::askScanForPlaylists(SourceFile const &file,
                                     unsigned int numOtherFiles)
  const {
  AskScanForPlaylistsDialog dialog{m_parent};
  return dialog.ask(file, numOtherFiles);
}

QList<SourceFilePtr>
PlaylistScanner::scanForPlaylists(QFileInfoList const &otherFiles)
  const {
  QProgressDialog progress{ QY("Scanning directory"), QY("Cancel"), 0, otherFiles.size(), m_parent };
  progress.setWindowModality(Qt::ApplicationModal);

  auto identifiedFiles = QList<SourceFilePtr>{};
  auto numScanned      = 0u;

  for (auto const &otherFile : otherFiles) {
    progress.setLabelText(QNY("%1 of %2 file processed", "%1 of %2 files processed", otherFiles.size()).arg(numScanned).arg(otherFiles.size()));
    progress.setValue(numScanned++);

    qApp->processEvents();
    if (progress.wasCanceled())
      return QList<SourceFilePtr>{};

    Util::FileIdentifier identifier{otherFile.filePath()};
    if (!identifier.identify()) {
      Util::MessageBox::critical(m_parent)->title(identifier.errorTitle()).text(identifier.errorText()).exec();
      continue;
    }

    auto file = identifier.file();
    if (file->isPlaylist() && (file->m_playlistDuration >= (Util::Settings::get().m_minimumPlaylistDuration * 1000000000ull)))
      identifiedFiles << file;
  }

  progress.setValue(otherFiles.size());

  std::sort(identifiedFiles.begin(), identifiedFiles.end(), [](SourceFilePtr const &a, SourceFilePtr const &b) { return a->m_fileName < b->m_fileName; });

  return identifiedFiles;
}

}}}
