#pragma once

#include "common/common_pch.h"

#include <QFileInfo>
#include <QStringList>
#include <QThread>

#include "mkvtoolnix-gui/merge/file_identification_pack.h"
#include "mkvtoolnix-gui/merge/source_file.h"

namespace mtx::gui::Merge {

class FileIdentificationWorkerPrivate;
class FileIdentificationWorker : public QObject {
  Q_OBJECT

protected:
  MTX_DECLARE_PRIVATE(FileIdentificationWorkerPrivate)

  std::unique_ptr<FileIdentificationWorkerPrivate> const p_ptr;

  explicit FileIdentificationWorker(FileIdentificationWorkerPrivate &p);

  enum class Result {
    Wait,
    Continue,
  };

public:
  FileIdentificationWorker(QObject *parent = nullptr);
  virtual ~FileIdentificationWorker();

  void addPackToIdentify(IdentificationPack &pack);

  void addIdentifiedFile(SourceFilePtr const &identifiedFile);
  void addIdentifiedFile(IdentificationPack::FileType type, QString const &fileName);
  void abortPlaylistScan();

  bool isEmpty() const;

public Q_SLOTS:
  void continueByScanningPlaylists(QFileInfoList const &files);
  void identifyFiles();
  void abortIdentification();

Q_SIGNALS:
  void queueStarted(unsigned int numberOfQueuedFiles);
  void queueFinished();
  void queueProgressChanged(unsigned int numFilesIdentified, unsigned int numberOfQueuedFiles);

  void playlistScanStarted(int numFilesToScan);
  void playlistScanFinished();
  void playlistScanProgressChanged(int numFilesScanned);
  void playlistScanDecisionNeeded(std::shared_ptr<SourceFile> sourceFile, QFileInfoList files);
  void playlistSelectionNeeded(QVector<std::shared_ptr<Merge::SourceFile>> identifiedPlaylists);

  void packIdentified(Merge::IdentificationPack const &pack);
  void identifiedAsPlaylist();

  void identificationFailed(QString const &errorTitle, QString const &errorText);

protected:
  IdentificationPack::FileType determineIfFileThatShouldBeSelectedElsewhere(QString const &fileName);
  std::optional<FileIdentificationWorker::Result> handleBlurayMainFile(QString const &fileName);
  std::optional<FileIdentificationWorker::Result> handleIdentifiedPlaylist(SourceFilePtr const &sourceFile);
  Result identifyThisFile(QString const &fileName);

  Result scanPlaylists(QFileInfoList const &fileNames);

  std::pair<unsigned int, unsigned int> countNumberOfIdentifiedAndQueuedFiles();
};

class FileIdentificationThread : public QThread {
  Q_OBJECT

protected:
  std::unique_ptr<FileIdentificationWorker> m_worker;

public:
  FileIdentificationThread(QObject *parent = nullptr);
  virtual ~FileIdentificationThread();

  FileIdentificationWorker &worker();

  bool isEmpty() const;

  void continueByScanningPlaylists(QFileInfoList const &files);
  void continueIdentification();
  void abortIdentification();

public Q_SLOTS:
  void abortPlaylistScan();
};

}
