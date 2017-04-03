#include "common/common_pch.h"

#include <iostream>

#include <QDir>
#include <QRegularExpression>
#include <QStringList>
#include <QTemporaryFile>
#include <QTimer>

#include "common/qt.h"
#include "mkvtoolnix-gui/jobs/mux_job.h"
#include "mkvtoolnix-gui/jobs/mux_job_p.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/merge/mux_config.h"
#include "mkvtoolnix-gui/merge/source_file.h"
#include "mkvtoolnix-gui/util/config_file.h"
#include "mkvtoolnix-gui/util/option_file.h"
#include "mkvtoolnix-gui/util/settings.h"

namespace mtx { namespace gui { namespace Jobs {

using namespace mtx::gui;

MuxJobPrivate::MuxJobPrivate(Job::Status pStatus,
                             mtx::gui::Merge::MuxConfigPtr const &pConfig)
  : JobPrivate{pStatus}
  , config{pConfig}
{
}

MuxJob::MuxJob(Status status,
               Merge::MuxConfigPtr const &config)
  : Job{*new MuxJobPrivate{status, config}}
{
  setupMuxJobConnections();
}

MuxJob::MuxJob(MuxJobPrivate &d)
  : Job{d}
{
  setupMuxJobConnections();
}

MuxJob::~MuxJob() {
}

void
MuxJob::setupMuxJobConnections() {
  Q_D(MuxJob);

  connect(&d->process, &QProcess::readyReadStandardOutput,                                              this, &MuxJob::readAvailable);
  connect(&d->process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, &MuxJob::processFinished);
  connect(&d->process, static_cast<void (QProcess::*)(QProcess::ProcessError)>(&QProcess::error),       this, &MuxJob::processError);
}

void
MuxJob::abort() {
  Q_D(MuxJob);

  if (d->aborted || (QProcess::NotRunning == d->process.state()))
    return;

  d->aborted = true;
  d->process.close();
}

void
MuxJob::start() {
  Q_D(MuxJob);

  d->aborted      = false;
  d->settingsFile = Util::OptionFile::createTemporary("MKVToolNix-GUI-MuxJob", d->config->buildMkvmergeOptions());

  setStatus(Job::Running);
  setProgress(0);

  d->process.start(Util::Settings::get().actualMkvmergeExe(), QStringList{} << "--gui-mode" << QString{"@%1"}.arg(d->settingsFile->fileName()), QIODevice::ReadOnly);
}

void
MuxJob::processBytesRead() {
  Q_D(MuxJob);

  d->bytesRead.replace("\r\n", "\n").replace('\r', '\n');

  auto start = 0, numRead = d->bytesRead.size();

  while (start < numRead) {
    auto pos = d->bytesRead.indexOf('\n', start);
    if (-1 == pos)
      break;

    processLine(QString::fromUtf8(d->bytesRead.mid(start, pos - start)));

    start = pos + 1;
  }

  d->bytesRead.remove(0, start);
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
  Q_D(MuxJob);

  d->bytesRead += d->process.readAllStandardOutput();
  processBytesRead();
}

void
MuxJob::processFinished(int exitCode,
                        QProcess::ExitStatus exitStatus) {
  Q_D(MuxJob);

  if (!d->bytesRead.isEmpty())
    processLine(QString::fromUtf8(d->bytesRead));

  d->exitCode = exitCode;
  auto status = d->aborted                         ? Job::Aborted
              : QProcess::NormalExit != exitStatus ? Job::Failed
              : 0 == exitCode                      ? Job::DoneOk
              : 1 == exitCode                      ? Job::DoneWarnings
              :                                      Job::Failed;

  setStatus(status);

  if (d->quitAfterFinished)
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
  Q_D(const MuxJob);

  QFileInfo info{d->config->m_destination};
  return QY("Multiplexing to file \"%1\" in directory \"%2\"").arg(info.fileName()).arg(QDir::toNativeSeparators(info.dir().path()));
}

QString
MuxJob::outputFolder()
  const {
  Q_D(const MuxJob);

  QFileInfo info{d->config->m_destination};
  return info.dir().path();
}

void
MuxJob::saveJobInternal(Util::ConfigFile &settings)
  const {
  Q_D(const MuxJob);

  settings.setValue("jobType", "MuxJob");
  settings.setValue("aborted", d->aborted);

  settings.beginGroup("muxConfig");
  d->config->save(settings);
  settings.endGroup();
}

JobPtr
MuxJob::loadMuxJob(Util::ConfigFile &settings) {
  auto config = std::make_shared<Merge::MuxConfig>();

  settings.beginGroup("muxConfig");
  config->load(settings);
  settings.endGroup();

  auto job                                                 = std::make_shared<MuxJob>(PendingManual, config);
  static_cast<MuxJobPrivate *>(job->d_ptr.data())->aborted = settings.value("aborted", false).toBool();
  job->loadJobBasis(settings);

  return std::static_pointer_cast<Job>(job);
}

Merge::MuxConfig const &
MuxJob::config()
  const {
  Q_D(const MuxJob);

  Q_ASSERT(!!d->config);
  return *d->config;
}

void
MuxJob::runProgramSetupVariables(ProgramRunner::VariableMap &variables)
  const{
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
