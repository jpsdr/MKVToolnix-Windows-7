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

namespace mtx::gui::Jobs {

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

MuxJob::MuxJob(MuxJobPrivate &p)
  : Job{p}
{
  setupMuxJobConnections();
}

MuxJob::~MuxJob() {
}

void
MuxJob::setupMuxJobConnections() {
  auto p = p_func();

  connect(&p->process, &QProcess::readyReadStandardOutput,                                              this, &MuxJob::readAvailable);
  connect(&p->process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, &MuxJob::processFinished);
  connect(&p->process, &QProcess::errorOccurred,                                                        this, &MuxJob::processError);
}

void
MuxJob::abort() {
  auto p = p_func();

  if (p->aborted || (QProcess::NotRunning == p->process.state()))
    return;

  p->aborted = true;
  p->process.close();
}

void
MuxJob::start() {
  auto p          = p_func();
  p->aborted      = false;
  p->settingsFile = Util::OptionFile::createTemporary("MKVToolNix-GUI-MuxJob", p->config->buildMkvmergeOptions().effectiveOptions(Util::EscapeJSON));

  setStatus(Job::Running);
  setProgress(0);

  p->process.start(Util::Settings::get().actualMkvmergeExe(), QStringList{} << "--gui-mode" << QString{"@%1"}.arg(p->settingsFile->fileName()), QIODevice::ReadOnly);
}

void
MuxJob::processBytesRead() {
  auto p = p_func();

  p->bytesRead.replace("\r\n", "\n").replace('\r', '\n');

  int start = 0, numRead = p->bytesRead.size();

  while (start < numRead) {
    auto pos = p->bytesRead.indexOf('\n', start);
    if (-1 == pos)
      break;

    processLine(QString::fromUtf8(p->bytesRead.mid(start, pos - start)));

    start = pos + 1;
  }

  p->bytesRead.remove(0, start);
}

void
MuxJob::processLine(QString const &rawLine) {
  auto line = rawLine;

  line.replace(QRegularExpression{"[\r\n]+"}, "");

  if (line.startsWith("#GUI#warning ")) {
    line.replace(QRegularExpression{"^#GUI#warning *"}, "");
    Q_EMIT lineRead(line, WarningLine);
    return;
  }

  if (line.startsWith("#GUI#error ")) {
    line.replace(QRegularExpression{"^#GUI#error *"}, "");
    Q_EMIT lineRead(line, ErrorLine);
    return;
  }

  if (line.startsWith("#GUI#begin_scanning_playlists")) {
    Q_EMIT startedScanningPlaylists();
    return;
  }

  if (line.startsWith("#GUI#end_scanning_playlists")) {
    Q_EMIT finishedScanningPlaylists();
    return;
  }

  auto matches = QRegularExpression{"^#GUI#progress\\s+(\\d+)%"}.match(line);
  if (matches.hasMatch()) {
    setProgress(matches.captured(1).toUInt());
    return;
  }

  Q_EMIT lineRead(line, InfoLine);
}

void
MuxJob::readAvailable() {
  auto p = p_func();

  p->bytesRead += p->process.readAllStandardOutput();
  processBytesRead();
}

void
MuxJob::processFinished(int exitCode,
                        QProcess::ExitStatus exitStatus) {
  auto p = p_func();

  if (!p->bytesRead.isEmpty())
    processLine(QString::fromUtf8(p->bytesRead));

  p->exitCode = exitCode;
  auto status = p->aborted                         ? Job::Aborted
              : QProcess::NormalExit != exitStatus ? Job::Failed
              : 0 == exitCode                      ? Job::DoneOk
              : 1 == exitCode                      ? Job::DoneWarnings
              :                                      Job::Failed;

  setStatus(status);

  if (p->quitAfterFinished)
    QTimer::singleShot(0, MainWindow::get(), []() { MainWindow::get()->close(); });
}

void
MuxJob::processError(QProcess::ProcessError error) {
  if (QProcess::FailedToStart == error)
    Q_EMIT lineRead(QY("The mkvmerge executable was not found."), ErrorLine);

  setStatus(Job::Failed);
}

QString
MuxJob::destinationFileName()
  const {
  return p_func()->config->m_destination;
}

QString
MuxJob::displayableType()
  const {
  return QY("Multiplex job");
}

QString
MuxJob::displayableDescription()
  const {
  QFileInfo info{p_func()->config->m_destination};
  return QY("Multiplexing to file \"%1\" in directory \"%2\"").arg(info.fileName()).arg(QDir::toNativeSeparators(info.dir().path()));
}

bool
MuxJob::isEditable()
  const {
  return true;
}

void
MuxJob::saveJobInternal(Util::ConfigFile &settings)
  const {
  auto p = p_func();

  settings.setValue("jobType", "MuxJob");
  settings.setValue("aborted", p->aborted);

  settings.beginGroup("muxConfig");
  p->config->save(settings);
  settings.endGroup();
}

JobPtr
MuxJob::loadMuxJob(Util::ConfigFile &settings) {
  auto config = std::make_shared<Merge::MuxConfig>();

  settings.beginGroup("muxConfig");
  config->load(settings);
  settings.endGroup();

  auto job               = std::make_shared<MuxJob>(PendingManual, config);
  job->p_func()->aborted = settings.value("aborted", false).toBool();
  job->loadJobBasis(settings);

  return std::static_pointer_cast<Job>(job);
}

Merge::MuxConfig const &
MuxJob::config()
  const {
  auto p = p_func();

  Q_ASSERT(!!p->config);
  return *p->config;
}

void
MuxJob::runProgramSetupVariables(ProgramRunner::VariableMap &variables)
  const{
  Job::runProgramSetupVariables(variables);

  auto &cfg = config();

  // OUTPUT_… are kept for backwards compatibility.
  variables[Q("JOB_TYPE")]                   << Q("multiplexer");
  variables[Q("OUTPUT_FILE_NAME")]           << QDir::toNativeSeparators(cfg.m_destination);
  variables[Q("OUTPUT_FILE_DIRECTORY")]      << QDir::toNativeSeparators(QFileInfo{ cfg.m_destination }.path());
  variables[Q("DESTINATION_FILE_NAME")]      << QDir::toNativeSeparators(cfg.m_destination);
  variables[Q("DESTINATION_FILE_DIRECTORY")] << QDir::toNativeSeparators(QFileInfo{ cfg.m_destination }.path());
  variables[Q("CHAPTERS_FILE_NAME")]         << QDir::toNativeSeparators(cfg.m_chapters);

  for (auto const &sourceFile : cfg.m_files) {
    variables[Q("SOURCE_FILE_NAMES")] << QDir::toNativeSeparators(sourceFile->m_fileName);

    for (auto const &appendedSourceFile : sourceFile->m_appendedFiles)
      variables[Q("SOURCE_FILE_NAMES")] << QDir::toNativeSeparators(appendedSourceFile->m_fileName);

    for (auto const &additionalPart : sourceFile->m_additionalParts)
      variables[Q("SOURCE_FILE_NAMES")] << QDir::toNativeSeparators(additionalPart->m_fileName);
  }
}

}
