#include "common/common_pch.h"

#include <QAtomicInt>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QTimer>

#include "common/qt.h"
#include "mkvtoolnix-gui/merge/file_identification_thread.h"
#include "mkvtoolnix-gui/merge/source_file.h"
#include "mkvtoolnix-gui/util/file_identifier.h"
#include "mkvtoolnix-gui/util/settings.h"

namespace mtx { namespace gui { namespace Merge {

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
  boost::regex m_simpleChaptersRE, m_xmlChaptersRE, m_xmlSegmentInfoRE, m_xmlTagsRE;

  explicit FileIdentificationWorkerPrivate()
  {
  }
};

using namespace mtx::gui;

FileIdentificationWorker::FileIdentificationWorker(QObject *parent)
  : QObject{parent}
  , d_ptr{new FileIdentificationWorkerPrivate{}}
{
  Q_D(FileIdentificationWorker);

  d->m_simpleChaptersRE = boost::regex{"^CHAPTER\\d{2}=.*CHAPTER\\d{2}NAME=",   boost::regex::perl | boost::regex::mod_s};
  d->m_xmlChaptersRE    = boost::regex{"<\\?xml[^>]+version.*\\?>.*<Chapters>", boost::regex::perl | boost::regex::mod_s};
  d->m_xmlSegmentInfoRE = boost::regex{"<\\?xml[^>]+version.*\\?>.*<Info>",     boost::regex::perl | boost::regex::mod_s};
  d->m_xmlTagsRE        = boost::regex{"<\\?xml[^>]+version.*\\?>.*<Tags>",     boost::regex::perl | boost::regex::mod_s};
}

FileIdentificationWorker::~FileIdentificationWorker() {
}

void
FileIdentificationWorker::addFilesToIdentify(QStringList const &fileNames,
                                             bool append,
                                             QModelIndex const &sourceFileIdx) {
  Q_D(FileIdentificationWorker);

  qDebug() << "FileIdentificationWorker::addFilesToIdentify: adding" << fileNames;

  QMutexLocker lock{&d->m_mutex};

  d->m_toIdentify.push_back({ fileNames, append, sourceFileIdx });

  QTimer::singleShot(0, this, SLOT(identifyFiles()));
}


void
FileIdentificationWorker::abortPlaylistScan() {
  Q_D(FileIdentificationWorker);

  qDebug() << "FileIdentificationWorker::abortPlaylistScan: setting flag";

  d->m_abortPlaylistScan = true;
}

void
FileIdentificationWorker::addIdentifiedFile(SourceFilePtr const &identifiedFile) {
  Q_D(FileIdentificationWorker);

  QMutexLocker lock{&d->m_mutex};
  d->m_toIdentify.first().m_identifiedFiles << identifiedFile;
}

void
FileIdentificationWorker::identifyFiles() {
  Q_D(FileIdentificationWorker);

  qDebug() << "FileIdentificationWorker::identifyFiles: starting loop";

  emit queueStarted();

  while (true) {
    QString fileName;

    {
      QMutexLocker lock{&d->m_mutex};
      if (d->m_toIdentify.isEmpty()) {
        qDebug() << "FileIdentificationWorker::identifyFiles: exiting loop (nothing left to do)";

        emit queueFinished();

        return;
      }

      auto &pack = d->m_toIdentify.first();

      if (pack.m_fileNames.isEmpty()) {
        qDebug() << "FileIdentificationWorker::identifyFiles: pack finished, notifying";

        emit filesIdentified(pack.m_identifiedFiles, pack.m_append, pack.m_sourceFileIdx);
        d->m_toIdentify.removeFirst();

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
  Q_D(FileIdentificationWorker);

  QFile file{fileName};
  if (!file.open(QIODevice::ReadOnly))
    return false;

  auto content = std::string{ file.read(1024).data() };

  if (boost::regex_search(content, d->m_simpleChaptersRE) || boost::regex_search(content, d->m_xmlChaptersRE))
    emit identifiedAsXmlOrSimpleChapters(fileName);

  else if (boost::regex_search(content, d->m_xmlSegmentInfoRE))
    emit identifiedAsXmlSegmentInfo(fileName);

  else if (boost::regex_search(content, d->m_xmlTagsRE))
    emit identifiedAsXmlTags(fileName);

  else
    return false;

  return true;
}

boost::optional<FileIdentificationWorker::Result>
FileIdentificationWorker::handleBluRayMainFile(QString const &fileName) {
  auto info = QFileInfo{fileName};

  if (info.completeSuffix().toLower() != Q("bdmv"))
    return {};

  auto dir = info.absoluteDir();
  if (!dir.cd("PLAYLIST") && !dir.cd("playlist"))
    return {};

  return scanPlaylists(dir.entryInfoList(QStringList{QString{"*.mpls"}, QString{"*.MPLS"}}, QDir::Files, QDir::Name));
}

boost::optional<FileIdentificationWorker::Result>
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
  Q_D(FileIdentificationWorker);

  auto numFiles = files.count();

  if (!numFiles)
    return Result::Continue;

  qDebug() << "FileIdentificationWorker::scanPlaylists: starting playlist scan, num files:" << numFiles;
  qDebug() << "FileIdentificationWorker::scanPlaylists: TID" << QThread::currentThreadId();

  d->m_abortPlaylistScan = false;

  emit playlistScanStarted(numFiles);

  QList<SourceFilePtr> identifiedPlaylists;

  for (auto idx = 0; idx < numFiles; ++idx) {
    Util::FileIdentifier identifier{files[idx].filePath()};
    if (identifier.identify())
      identifiedPlaylists << identifier.file();
    else
      qDebug() << "the error of my ways" << identifier.errorTitle() << identifier.errorText();

    if (d->m_abortPlaylistScan) {
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

  auto result = handleBluRayMainFile(fileName);
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

}}}
