#include "common/common_pch.h"

#include <iostream>

#include <QDir>
#include <QRegularExpression>
#include <QStringList>
#include <QTemporaryFile>
#include <QTimer>

#include "common/qt.h"
#include "mkvtoolnix-gui/jobs/mux_job.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/merge/mux_config.h"
#include "mkvtoolnix-gui/merge/source_file.h"
#include "mkvtoolnix-gui/util/config_file.h"
#include "mkvtoolnix-gui/util/option_file.h"
#include "mkvtoolnix-gui/util/settings.h"

namespace mtx { namespace gui { namespace Jobs {

using namespace mtx::gui;

MuxJob::MuxJob(Status status,
               Merge::MuxConfigPtr const &config)
  : Job{status}
  , m_config{config}
  , m_aborted{}
{
 connect(&m_process, &QProcess::readyReadStandardOutput,                                              this, &MuxJob::readAvailable);
 connect(&m_process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, &MuxJob::processFinished);
 connect(&m_process, static_cast<void (QProcess::*)(QProcess::ProcessError)>(&QProcess::error),       this, &MuxJob::processError);
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
  m_settingsFile = Util::OptionFile::createTemporary("MKVToolNix-GUI-MuxJob", m_config->buildMkvmergeOptions());

  setStatus(Job::Running);
  setProgress(0);

  m_process.start(Util::Settings::get().actualMkvmergeExe(), QStringList{} << "--gui-mode" << QString{"@%1"}.arg(m_settingsFile->fileName()), QIODevice::ReadOnly);
}

void
MuxJob::processBytesRead() {
  m_bytesRead.replace("\r\n", "\n").replace('\r', '\n');

  auto start = 0, numRead = m_bytesRead.size();

  while (start < numRead) {
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

  line.replace(QRegularExpression{"[\r\n]+"}, "");

  if (line.startsWith("#GUI#warning ")) {
    line.replace(QRegularExpression{"^#GUI#warning *"}, "");
    emit lineRead(line, WarningLine);
    return;
  }

  if (line.startsWith("#GUI#error ")) {
    line.replace(QRegularExpression{"^#GUI#error *"}, "");
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

  auto matches = QRegularExpression{"^#GUI#progress\\s+(\\d+)%"}.match(line);
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

  m_exitCode  = exitCode;
  auto status = m_aborted                          ? Job::Aborted
              : QProcess::NormalExit != exitStatus ? Job::Failed
              : 0 == exitCode                      ? Job::DoneOk
              : 1 == exitCode                      ? Job::DoneWarnings
              :                                      Job::Failed;

  setStatus(status);

  if (m_quitAfterFinished)
    QTimer::singleShot(0, MainWindow::get(), SLOT(close()));
}

void
MuxJob::processError(QProcess::ProcessError error) {
  if (QProcess::FailedToStart == error)
    emit lineRead(QY("The mkvmerge executable was not found."), ErrorLine);

  setStatus(Job::Failed);
}

QString
MuxJob::displayableType()
  const {
  return QY("Multiplex job");
}

QString
MuxJob::displayableDescription()
  const {
  QFileInfo info{m_config->m_destination};
  return QY("Multiplexing to file \"%1\" in directory \"%2\"").arg(info.fileName()).arg(QDir::toNativeSeparators(info.dir().path()));
}

QString
MuxJob::outputFolder()
  const {
  QFileInfo info{m_config->m_destination};
  return info.dir().path();
}

void
MuxJob::saveJobInternal(Util::ConfigFile &settings)
  const {
  settings.setValue("jobType", "MuxJob");
  settings.setValue("aborted", m_aborted);

  settings.beginGroup("muxConfig");
  m_config->save(settings);
  settings.endGroup();
}

JobPtr
MuxJob::loadMuxJob(Util::ConfigFile &settings) {
  auto config = std::make_shared<Merge::MuxConfig>();

  settings.beginGroup("muxConfig");
  config->load(settings);
  settings.endGroup();

  auto job       = new MuxJob{PendingManual, config};
  job->m_aborted = settings.value("aborted", false).toBool();
  job->loadJobBasis(settings);

  return JobPtr{ job };
}

Merge::MuxConfig const &
MuxJob::config()
  const {
  Q_ASSERT(!!m_config);
  return *m_config;
}

void
MuxJob::runProgramSetupVariables(ProgramRunner::VariableMap &variables) {
  Job::runProgramSetupVariables(variables);

  // OUTPUT_… are kept for backwards compatibility.
  variables[Q("OUTPUT_FILE_NAME")]           << QDir::toNativeSeparators(config().m_destination);
  variables[Q("OUTPUT_FILE_DIRECTORY")]      << QDir::toNativeSeparators(QFileInfo{ config().m_destination }.path());
  variables[Q("DESTINATION_FILE_NAME")]      << QDir::toNativeSeparators(config().m_destination);
  variables[Q("DESTINATION_FILE_DIRECTORY")] << QDir::toNativeSeparators(QFileInfo{ config().m_destination }.path());

  for (auto const &sourceFile : config().m_files)
    variables[Q("SOURCE_FILE_NAMES")] << QDir::toNativeSeparators(sourceFile->m_fileName);
}

}}}
