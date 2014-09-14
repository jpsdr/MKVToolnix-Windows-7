#include "common/common_pch.h"

#include <QRegularExpression>
#include <QSettings>
#include <QStringList>
#include <QTemporaryFile>

#include "common/qt.h"
#include "mkvtoolnix-gui/job_widget/mux_job.h"
#include "mkvtoolnix-gui/merge_widget/mux_config.h"
#include "mkvtoolnix-gui/util/option_file.h"
#include "mkvtoolnix-gui/util/settings.h"

MuxJob::MuxJob(Status status,
               MuxConfigPtr const &config)
  : Job{status}
  , m_config{config}
  , m_aborted{}
{
 connect(&m_process, SIGNAL(readyReadStandardOutput()),          this, SLOT(readAvailable()));
 connect(&m_process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(processFinished(int,QProcess::ExitStatus)));
 connect(&m_process, SIGNAL(error(QProcess::ProcessError)),      this, SLOT(processError(QProcess::ProcessError)));
}

MuxJob::~MuxJob() {
}

void
MuxJob::abort() {
  if (m_aborted || (QProcess::NotRunning == m_process.state()))
    return;

  m_aborted = true;
  m_process.close();
}

void
MuxJob::start() {
  m_aborted      = false;
  m_settingsFile = OptionFile::createTemporary("MKVToolNix-GUI-MuxJob-XXXXXX", m_config->buildMkvmergeOptions());

  setStatus(Job::Running);
  setProgress(0);

  m_process.start(Settings::get().actualMkvmergeExe(), QStringList{} << "--gui-mode" << QString{"@%1"}.arg(m_settingsFile->fileName()), QIODevice::ReadOnly);
}

void
MuxJob::processBytesRead() {
  m_bytesRead.replace('\r', '\n').replace("\r\n", "\n");

  auto start = 0, num_read = m_bytesRead.size();

  while (start < num_read) {
    auto pos = m_bytesRead.indexOf('\n', start);
    if (-1 == pos)
      break;

    processLine(QString::fromUtf8(m_bytesRead.mid(start, pos - start)));

    start = pos + 1;
  }

  m_bytesRead.remove(0, start);
}

void
MuxJob::processLine(QString const &rawLine) {
  auto line = rawLine;

  line.replace(QRegularExpression{"[\r\n]+$"}, "");
  std::cout << "processLine: " << to_utf8(line) << std::endl;

  // TODO: MuxJob::processLine
  if (line.startsWith("Warning:")) {
    line.replace(QRegularExpression{"^Warning: *"}, "");
    emit lineRead(line, WarningLine);
    return;
  }

  if (line.startsWith("Error:")) {
    line.replace(QRegularExpression{"^Error: *"}, "");
    emit lineRead(line, ErrorLine);
    return;
  }

  if (line.startsWith("#GUI#begin_scanning_playlists")) {
    emit startedScanningPlaylists();
    return;
  }

  if (line.startsWith("#GUI#end_scanning_playlists")) {
    emit finishedScanningPlaylists();
    return;
  }

  auto matches = QRegularExpression{"^Progress: *(\\d+)%"}.match(line);
  if (matches.hasMatch()) {
    setProgress(matches.captured(1).toUInt());
    return;
  }

  emit lineRead(line, InfoLine);
}

void
MuxJob::readAvailable() {
  m_bytesRead += m_process.readAllStandardOutput();
  processBytesRead();
}

void
MuxJob::processFinished(int exitCode,
                        QProcess::ExitStatus exitStatus) {
  if (!m_bytesRead.isEmpty())
    processLine(QString::fromUtf8(m_bytesRead));
  // TODO: MuxJob::processFinished

  auto status = m_aborted                          ? Job::Aborted
              : QProcess::NormalExit != exitStatus ? Job::Failed
              : 0 == exitCode                      ? Job::DoneOk
              : 1 == exitCode                      ? Job::DoneWarnings
              :                                      Job::Failed;

  setStatus(status);
}

void
MuxJob::processError(QProcess::ProcessError /*error*/) {
  setStatus(Job::Failed);
}

QString
MuxJob::displayableType()
  const {
  return QY("merge");
}

QString
MuxJob::displayableDescription()
  const {
  QFileInfo info{m_config->m_destination};
  return QY("merging to file »%1« in directory »%2«").arg(info.fileName()).arg(info.filePath());
}

void
MuxJob::saveJobInternal(QSettings &settings)
  const {
  settings.setValue("jobType", "MuxJob");
  settings.setValue("aborted", m_aborted);

  settings.beginGroup("muxConfig");
  m_config->save(settings);
  settings.endGroup();
}

JobPtr
MuxJob::loadMuxJob(QSettings &settings) {
  auto config = std::make_shared<MuxConfig>();

  settings.beginGroup("muxConfig");
  config->load(settings);
  settings.endGroup();

  auto job       = new MuxJob{PendingManual, config};
  job->m_aborted = settings.value("aborted", false).toBool();
  job->loadJobBasis(settings);

  return JobPtr{ job };
}
