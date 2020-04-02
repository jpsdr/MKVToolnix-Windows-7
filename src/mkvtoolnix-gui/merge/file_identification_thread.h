#pragma once

#include "common/common_pch.h"

#include <QFileInfo>
#include <QModelIndex>
#include <QStringList>
#include <QThread>

#include "mkvtoolnix-gui/merge/source_file.h"

namespace mtx::gui::Merge {

class SourceFile;

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

  void addFilesToIdentify(QStringList const &fileNames, bool append, QModelIndex const &sourceFileIdx);

  void addIdentifiedFile(std::shared_ptr<SourceFile> const &identifiedFile);
  void abortPlaylistScan();

  bool isEmpty() const;

public slots:
  void continueByScanningPlaylists(QFileInfoList const &files);

protected slots:
  void identifyFiles();
  void abortIdentification();

signals:
  void queueStarted();
  void queueFinished();

  void playlistScanStarted(int numFilesToScan);
  void playlistScanFinished();
  void playlistScanProgressChanged(int numFilesScanned);
  void playlistScanDecisionNeeded(std::shared_ptr<SourceFile> sourceFile, QFileInfoList files);
  void playlistSelectionNeeded(QList<std::shared_ptr<Merge::SourceFile>> identifiedPlaylists);

  void filesIdentified(QList<std::shared_ptr<Merge::SourceFile>> identifiedFiles, bool append, QModelIndex sourceFileIdx);

  void identifiedAsXmlOrSimpleChapters(QString const &fileName);
  void identifiedAsXmlSegmentInfo(QString const &fileName);
  void identifiedAsXmlTags(QString const &fileName);
  void identifiedAsPlaylist();

  void identificationFailed(QString const &errorTitle, QString const &errorText);

protected:
  bool handleFileThatShouldBeSelectedElsewhere(QString const &fileName);
  std::optional<FileIdentificationWorker::Result> handleBlurayMainFile(QString const &fileName);
  std::optional<FileIdentificationWorker::Result> handleIdentifiedPlaylist(SourceFilePtr const &sourceFile);
  Result identifyThisFile(QString const &fileName);

  Result scanPlaylists(QFileInfoList const &fileNames);
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

public slots:
  void abortPlaylistScan();
};

}
