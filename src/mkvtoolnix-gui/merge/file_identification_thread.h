#pragma once

#include "common/common_pch.h"

#include <QFileInfo>
#include <QModelIndex>
#include <QStringList>
#include <QThread>

#include "mkvtoolnix-gui/merge/source_file.h"

namespace mtx { namespace gui { namespace Merge {

using namespace mtx::gui;

class SourceFile;

class FileIdentificationWorkerPrivate;
class FileIdentificationWorker : public QObject {
  Q_OBJECT;

  enum class Result {
    Wait,
    Continue,
  };

protected:
  Q_DECLARE_PRIVATE(FileIdentificationWorker);

  QScopedPointer<FileIdentificationWorkerPrivate> const d_ptr;

  explicit FileIdentificationWorker(FileIdentificationWorkerPrivate &d);

public:
  FileIdentificationWorker(QObject *parent = nullptr);
  virtual ~FileIdentificationWorker();

  void addFilesToIdentify(QStringList const &fileNames, bool append, QModelIndex const &sourceFileIdx);

  void addIdentifiedFile(std::shared_ptr<SourceFile> const &identifiedFile);
  void abortPlaylistScan();

public slots:
  void continueByScanningPlaylists(QFileInfoList const &files);

protected slots:
  void identifyFiles();

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
  boost::optional<FileIdentificationWorker::Result> handleBluRayMainFile(QString const &fileName);
  boost::optional<FileIdentificationWorker::Result> handleIdentifiedPlaylist(SourceFilePtr const &sourceFile);
  Result identifyThisFile(QString const &fileName);

  Result scanPlaylists(QFileInfoList const &fileNames);
};

class FileIdentificationThread : public QThread {
  Q_OBJECT;

protected:
  QScopedPointer<FileIdentificationWorker> m_worker;

public:
  FileIdentificationThread(QObject *parent = nullptr);
  virtual ~FileIdentificationThread();

  FileIdentificationWorker &worker();

  void continueByScanningPlaylists(QFileInfoList const &files);
  void continueIdentification();

public slots:
  void abortPlaylistScan();
};

}}}
