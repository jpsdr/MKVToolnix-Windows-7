#include "common/common_pch.h"

#include <QAtomicInt>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QRegularExpression>
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

  QVector<IdentificationPack> m_toIdentify;
  QMutex m_mutex;
  QAtomicInteger<bool> m_abortPlaylistScan;
  QRegularExpression m_simpleChaptersRE, m_xmlChaptersRE, m_xmlSegmentInfoRE, m_xmlTagsRE;

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
  p->m_simpleChaptersRE = QRegularExpression{R"(^CHAPTER\d{2}=[\s\S]*CHAPTER\d{2}NAME=)"};
  p->m_xmlChaptersRE    = QRegularExpression{R"(<\?xml[^>]+version[\s\S]*\?>[\s\S]*<Chapters>)"};
  p->m_xmlSegmentInfoRE = QRegularExpression{R"(<\?xml[^>]+version[\s\S]*\?>[\s\S]*<Info>)"};
  p->m_xmlTagsRE        = QRegularExpression{R"(<\?xml[^>]+version[\s\S]*\?>[\s\S]*<Tags>)"};
}

FileIdentificationWorker::~FileIdentificationWorker() {
}

void
FileIdentificationWorker::addPackToIdentify(IdentificationPack const &pack) {
  auto p = p_func();

  qDebug() << "FileIdentificationWorker::addFilesToIdentify: adding" << pack.m_fileNames;

  QMutexLocker lock{&p->m_mutex};

  p->m_toIdentify.push_back(pack);

  QTimer::singleShot(0, this, [this]() { identifyFiles(); });
}


void
FileIdentificationWorker::abortPlaylistScan() {
  qDebug() << "FileIdentificationWorker::abortPlaylistScan: setting flag";

  p_func()->m_abortPlaylistScan = true;
}

void
FileIdentificationWorker::addIdentifiedFile(SourceFilePtr const &sourceFile) {
  auto p = p_func();

  QMutexLocker lock{&p->m_mutex};
  p->m_toIdentify.first().m_identifiedFiles << IdentificationPack::IdentifiedFile{ IdentificationPack::FileType::Regular, sourceFile->m_fileName, sourceFile };
}

void
FileIdentificationWorker::addIdentifiedFile(IdentificationPack::FileType type,
                                            QString const &fileName) {
  auto p = p_func();

  QMutexLocker lock{&p->m_mutex};
  p->m_toIdentify.first().m_identifiedFiles << IdentificationPack::IdentifiedFile{ type, fileName, {} };
}

bool
FileIdentificationWorker::isEmpty()
  const {
  auto const &toIdentify = p_func()->m_toIdentify;

  if (toIdentify.isEmpty())
    return true;

  if (toIdentify.size() != 1)
    return false;

  return toIdentify.first().m_fileNames.isEmpty();
}

void
FileIdentificationWorker::identifyFiles() {
  auto p = p_func();

  qDebug() << "FileIdentificationWorker::identifyFiles: starting loop";

  Q_EMIT queueStarted();

  while (true) {
    QString fileName;

    {
      QMutexLocker lock{&p->m_mutex};
      if (p->m_toIdentify.isEmpty()) {
        qDebug() << "FileIdentificationWorker::identifyFiles: exiting loop (nothing left to do)";

        Q_EMIT queueFinished();

        return;
      }

      auto &pack = p->m_toIdentify.first();

      if (pack.m_fileNames.isEmpty()) {
        qDebug() << "FileIdentificationWorker::identifyFiles: pack finished, notifying";

        Q_EMIT packIdentified(pack);

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

void
FileIdentificationWorker::abortIdentification() {
  auto p = p_func();

  QMutexLocker lock{&p->m_mutex};
  if (p->m_toIdentify.isEmpty())
    return;

  qDebug() << "FileIdentificationWorker::abortIdentification: skipping remaining files";

  p->m_toIdentify.clear();

  Q_EMIT queueFinished();
}

IdentificationPack::FileType
FileIdentificationWorker::determineIfFileThatShouldBeSelectedElsewhere(QString const &fileName) {
  auto p = p_func();

  QFile file{fileName};
  if (!file.open(QIODevice::ReadOnly))
    return IdentificationPack::FileType::Regular;

  auto content = QString::fromUtf8(file.read(1024 * 10));

  if (content.contains(p->m_simpleChaptersRE) || content.contains(p->m_xmlChaptersRE))
    return IdentificationPack::FileType::Chapters;

  else if (content.contains(p->m_xmlSegmentInfoRE))
    return IdentificationPack::FileType::SegmentInfo;

  else if (content.contains(p->m_xmlTagsRE))
    return IdentificationPack::FileType::Tags;

  return IdentificationPack::FileType::Regular;
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

  Q_EMIT playlistScanDecisionNeeded(sourceFile, files);

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

  Q_EMIT playlistScanStarted(numFiles);

  QVector<SourceFilePtr> identifiedPlaylists;
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

      Q_EMIT playlistScanFinished();

      return Result::Continue;
    }

    Q_EMIT playlistScanProgressChanged(idx + 1);
  }

  Q_EMIT playlistScanProgressChanged(numFiles);
  Q_EMIT playlistScanFinished();

  if (identifiedPlaylists.isEmpty()) {
    qDebug() << "FileIdentificationWorker::scanPlaylists: scan finished, no files";
    return Result::Continue;
  }

  qDebug() << "FileIdentificationWorker::scanPlaylists: scan finished with one or more files, potentially requiring user selection";

  Q_EMIT playlistSelectionNeeded(identifiedPlaylists);

  return Result::Wait;
}

FileIdentificationWorker::Result
FileIdentificationWorker::identifyThisFile(QString const &fileName) {
  qDebug() << "FileIdentificationWorker::identifyThisFile: starting for" << fileName;
  qDebug() << "FileIdentificationWorker::identifyThisFile: thread ID:" << QThread::currentThreadId();

  auto fileType = determineIfFileThatShouldBeSelectedElsewhere(fileName);
  if (fileType != IdentificationPack::FileType::Regular) {
    qDebug() << "FileIdentificationWorker::identifyThisFile: identified as chapters/tags/segmentinfo";
    addIdentifiedFile(fileType, fileName);
    return Result::Continue;
  }

  auto result = handleBlurayMainFile(fileName);
  if (result) {
    qDebug() << "FileIdentificationWorker::identifyThisFile: identified as Blu-ray index.bdmv/MovieObject.bdmv";
    return *result;
  }

  Util::FileIdentifier identifier{fileName};
  if (!identifier.identify()) {
    qDebug() << "FileIdentificationWorker::identifyThisFile: failed";
    Q_EMIT identificationFailed(identifier.errorTitle(), identifier.errorText());
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
  QTimer::singleShot(0, &worker(), [this]() { worker().identifyFiles(); });
}

void
FileIdentificationThread::abortIdentification() {
  QTimer::singleShot(0, &worker(), [this]() { worker().abortIdentification(); });
}

void
FileIdentificationThread::continueByScanningPlaylists(QFileInfoList const &fileNames) {
  QMetaObject::invokeMethod(&worker(), "continueByScanningPlaylists", Q_ARG(QFileInfoList, fileNames));
}

bool
FileIdentificationThread::isEmpty()
  const {
  return m_worker->isEmpty();
}

}
