#include "common/common_pch.h"

#include <QAtomicInt>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QTimer>

#include "common/qt.h"
#include "common/timestamp.h"
#include "mkvtoolnix-gui/merge/file_identification_thread.h"
#include "mkvtoolnix-gui/merge/source_file.h"
#include "mkvtoolnix-gui/util/file_identifier.h"
#include "mkvtoolnix-gui/util/settings.h"

namespace mtx::gui::Merge {

class FileIdentificationWorkerPrivate {
  friend class FileIdentificationWorker;

  struct IdentificationPack {
    QStringList m_fileNames;
    bool m_append;
    QModelIndex m_sourceFileIdx;
    QList<SourceFilePtr> m_identifiedFiles;
  };

  QList<IdentificationPack> m_toIdentify;
  QMutex m_mutex;
  QAtomicInteger<bool> m_abortPlaylistScan;
  std::regex m_simpleChaptersRE, m_xmlChaptersRE, m_xmlSegmentInfoRE, m_xmlTagsRE;

  explicit FileIdentificationWorkerPrivate()
  {
  }
};

using namespace mtx::gui;

FileIdentificationWorker::FileIdentificationWorker(QObject *parent)
  : QObject{parent}
  , p_ptr{new FileIdentificationWorkerPrivate{}}
{
  auto p                = p_func();
  p->m_simpleChaptersRE = std::regex{"^CHAPTER\\d{2}=[\\s\\S]*CHAPTER\\d{2}NAME="};
  p->m_xmlChaptersRE    = std::regex{"<\\?xml[^>]+version[\\s\\S]*\\?>.*<Chapters>"};
  p->m_xmlSegmentInfoRE = std::regex{"<\\?xml[^>]+version[\\s\\S]*\\?>.*<Info>"};
  p->m_xmlTagsRE        = std::regex{"<\\?xml[^>]+version[\\s\\S]*\\?>.*<Tags>"};
}

FileIdentificationWorker::~FileIdentificationWorker() {
}

void
FileIdentificationWorker::addFilesToIdentify(QStringList const &fileNames,
                                             bool append,
                                             QModelIndex const &sourceFileIdx) {
  auto p = p_func();

  qDebug() << "FileIdentificationWorker::addFilesToIdentify: adding" << fileNames;

  QMutexLocker lock{&p->m_mutex};

  p->m_toIdentify.push_back({ fileNames, append, sourceFileIdx });

  QTimer::singleShot(0, this, SLOT(identifyFiles()));
}


void
FileIdentificationWorker::abortPlaylistScan() {
  qDebug() << "FileIdentificationWorker::abortPlaylistScan: setting flag";

  p_func()->m_abortPlaylistScan = true;
}

void
FileIdentificationWorker::addIdentifiedFile(SourceFilePtr const &identifiedFile) {
  auto p = p_func();

  QMutexLocker lock{&p->m_mutex};
  p->m_toIdentify.first().m_identifiedFiles << identifiedFile;
}

void
FileIdentificationWorker::identifyFiles() {
  auto p = p_func();

  qDebug() << "FileIdentificationWorker::identifyFiles: starting loop";

  emit queueStarted();

  while (true) {
    QString fileName;

    {
      QMutexLocker lock{&p->m_mutex};
      if (p->m_toIdentify.isEmpty()) {
        qDebug() << "FileIdentificationWorker::identifyFiles: exiting loop (nothing left to do)";

        emit queueFinished();

        return;
      }

      auto &pack = p->m_toIdentify.first();

      if (pack.m_fileNames.isEmpty()) {
        qDebug() << "FileIdentificationWorker::identifyFiles: pack finished, notifying";

        emit filesIdentified(pack.m_identifiedFiles, pack.m_append, pack.m_sourceFileIdx);
        p->m_toIdentify.removeFirst();

        continue;
      }

      fileName = pack.m_fileNames.takeFirst();
    }

    auto result = identifyThisFile(fileName);

    if (result == Result::Wait) {
      qDebug() << "FileIdentificationWorker::identifyFiles: exiting loop (result was 'Wait')";
      return;
    }
  }
}

bool
FileIdentificationWorker::handleFileThatShouldBeSelectedElsewhere(QString const &fileName) {
  auto p = p_func();

  QFile file{fileName};
  if (!file.open(QIODevice::ReadOnly))
    return false;

  auto content = std::string{ file.read(1024).data() };

  if (std::regex_search(content, p->m_simpleChaptersRE) || std::regex_search(content, p->m_xmlChaptersRE))
    emit identifiedAsXmlOrSimpleChapters(fileName);

  else if (std::regex_search(content, p->m_xmlSegmentInfoRE))
    emit identifiedAsXmlSegmentInfo(fileName);

  else if (std::regex_search(content, p->m_xmlTagsRE))
    emit identifiedAsXmlTags(fileName);

  else
    return false;

  return true;
}

std::optional<FileIdentificationWorker::Result>
FileIdentificationWorker::handleBlurayMainFile(QString const &fileName) {
  auto info = QFileInfo{fileName};

  if (info.completeSuffix().toLower() != Q("bdmv"))
    return {};

  auto dir = info.absoluteDir();
  if (!dir.cd("PLAYLIST") && !dir.cd("playlist"))
    return {};

  return scanPlaylists(dir.entryInfoList(QStringList{QString{"*.mpls"}, QString{"*.MPLS"}}, QDir::Files, QDir::Name));
}

std::optional<FileIdentificationWorker::Result>
FileIdentificationWorker::handleIdentifiedPlaylist(SourceFilePtr const &sourceFile) {
  if (!sourceFile->isPlaylist())
    return {};

  auto info   = QFileInfo{sourceFile->m_fileName};
  auto files  = QDir{info.path()}.entryInfoList(QStringList{QString{"*.%1"}.arg(info.suffix())}, QDir::Files, QDir::Name);
  auto policy = Util::Settings::get().m_scanForPlaylistsPolicy;

  if (   (files.count() < 2)
      || (Util::Settings::NeverScan == policy))
    return {};

  if (Util::Settings::AlwaysScan == policy)
    return scanPlaylists(files);

  emit playlistScanDecisionNeeded(sourceFile, files);

  return Result::Wait;
}

void
FileIdentificationWorker::continueByScanningPlaylists(QFileInfoList const &files) {
  auto result = scanPlaylists(files);

  if (result == Result::Continue)
    identifyFiles();
}

FileIdentificationWorker::Result
FileIdentificationWorker::scanPlaylists(QFileInfoList const &files) {
  auto p        = p_func();
  auto numFiles = files.count();

  if (!numFiles)
    return Result::Continue;

  qDebug() << "FileIdentificationWorker::scanPlaylists: starting playlist scan, num files:" << numFiles;
  qDebug() << "FileIdentificationWorker::scanPlaylists: TID" << QThread::currentThreadId();

  p->m_abortPlaylistScan = false;

  emit playlistScanStarted(numFiles);

  QList<SourceFilePtr> identifiedPlaylists;
  auto minimumPlaylistDuration = timestamp_c::s(Util::Settings::get().m_minimumPlaylistDuration);

  for (auto idx = 0; idx < numFiles; ++idx) {
    Util::FileIdentifier identifier{files[idx].filePath()};
    if (identifier.identify()) {
      auto file = identifier.file();
      if (timestamp_c::ns(file->m_playlistDuration) >= minimumPlaylistDuration)
        identifiedPlaylists << file;

    } else
      qDebug() << "the error of my ways" << identifier.errorTitle() << identifier.errorText();

    if (p->m_abortPlaylistScan) {
      qDebug() << "FileIdentificationWorker::scanPlaylists: scan aborted";

      emit playlistScanFinished();

      return Result::Continue;
    }

    emit playlistScanProgressChanged(idx + 1);
  }

  emit playlistScanProgressChanged(numFiles);
  emit playlistScanFinished();

  if (identifiedPlaylists.isEmpty()) {
    qDebug() << "FileIdentificationWorker::scanPlaylists: scan finished, no files";
    return Result::Continue;
  }

  if (identifiedPlaylists.count() == 1) {
    qDebug() << "FileIdentificationWorker::scanPlaylists: scan finished, exactly one file, adding directly";
    addIdentifiedFile(identifiedPlaylists.first());
    return Result::Continue;
  }

  qDebug() << "FileIdentificationWorker::scanPlaylists: scan finished, multiple files, requiring user selection";

  emit playlistSelectionNeeded(identifiedPlaylists);

  return Result::Wait;
}

FileIdentificationWorker::Result
FileIdentificationWorker::identifyThisFile(QString const &fileName) {
  qDebug() << "FileIdentificationWorker::identifyThisFile: starting for" << fileName;
  qDebug() << "FileIdentificationWorker::identifyThisFile: thread ID:" << QThread::currentThreadId();

  if (handleFileThatShouldBeSelectedElsewhere(fileName)) {
    qDebug() << "FileIdentificationWorker::identifyThisFile: identified as chapters/tags/segmentinfo";
    return Result::Wait;
  }

  auto result = handleBlurayMainFile(fileName);
  if (result) {
    qDebug() << "FileIdentificationWorker::identifyThisFile: identified as Blu-ray index.bdmv/MovieObject.bdmv";
    return *result;
  }

  Util::FileIdentifier identifier{fileName};
  if (!identifier.identify()) {
    qDebug() << "FileIdentificationWorker::identifyThisFile: failed";
    emit identificationFailed(identifier.errorTitle(), identifier.errorText());
    return Result::Wait;
  }

  result = handleIdentifiedPlaylist(identifier.file());
  if (result) {
    qDebug() << "FileIdentificationWorker::identifyThisFile: identified as playlist & handled accordingly";
    return *result;
  }

  addIdentifiedFile(identifier.file());

  return Result::Continue;
}

// ----------------------------------------------------------------------

FileIdentificationThread::FileIdentificationThread(QObject *parent)
  : QThread{parent}
  , m_worker{new FileIdentificationWorker{}}
{
  m_worker->moveToThread(this);
}

FileIdentificationThread::~FileIdentificationThread() {
}

FileIdentificationWorker &
FileIdentificationThread::worker() {
  return *m_worker;
}

void
FileIdentificationThread::abortPlaylistScan() {
  worker().abortPlaylistScan();
}

void
FileIdentificationThread::continueIdentification() {
  QTimer::singleShot(0, &worker(), SLOT(identifyFiles()));
}

void
FileIdentificationThread::continueByScanningPlaylists(QFileInfoList const &fileNames) {
  QMetaObject::invokeMethod(&worker(), "continueByScanningPlaylists", Q_ARG(QFileInfoList, fileNames));
}

}
