#include "common/common_pch.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QSettings>
#include <QUrl>

#include "common/list_utils.h"
#include "common/logger.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/jobs/info_job.h"
#include "mkvtoolnix-gui/jobs/job.h"
#include "mkvtoolnix-gui/jobs/job_p.h"
#include "mkvtoolnix-gui/jobs/mux_job.h"
#include "mkvtoolnix-gui/merge/mux_config.h"
#include "mkvtoolnix-gui/util/config_file.h"
#include "mkvtoolnix-gui/util/file.h"
#include "mkvtoolnix-gui/util/settings.h"

namespace mtx { namespace gui { namespace Jobs {

static uint64_t s_next_id = 0;

JobPrivate::JobPrivate(Job::Status pStatus)
  : id{s_next_id++}
  , status{pStatus}
{
}

Job::Job(Status status)
  : p_ptr{new JobPrivate{status}}
{
  setupJobConnections();
}

Job::Job(JobPrivate &p)
  : p_ptr{&p}
{
  setupJobConnections();
}

Job::~Job() {
}

void
Job::setupJobConnections() {
  connect(this, &Job::lineRead, this, &Job::addLineToInternalLogs);
}

uint64_t
Job::id()
  const {
  return p_func()->id;
}

QUuid
Job::uuid()
  const {
  return p_func()->uuid;
}

Job::Status
Job::status()
  const {
  return p_func()->status;
}

QString
Job::description()
  const {
  return p_func()->description;
}

unsigned int
Job::progress()
  const {
  return p_func()->progress;
}

QStringList const &
Job::output()
  const {
  return p_func()->output;
}

QStringList const &
Job::warnings()
  const {
  return p_func()->warnings;
}

QStringList const &
Job::errors()
  const {
  return p_func()->errors;
}

QStringList const &
Job::fullOutput()
  const {
  return p_func()->fullOutput;
}

QDateTime
Job::dateAdded()
  const {
  return p_func()->dateAdded;
}

QDateTime
Job::dateStarted()
  const {
  return p_func()->dateStarted;
}

QDateTime
Job::dateFinished()
  const {
  return p_func()->dateFinished;
}

bool
Job::isModified()
  const {
  return p_func()->modified;
}

void
Job::setDescription(QString const &pDescription) {
  auto p         = p_func();
  p->description = pDescription;
  p->modified    = true;
}

void
Job::setDateAdded(QDateTime const &pDateAdded) {
  auto p       = p_func();
  p->dateAdded = pDateAdded;
  p->modified  = true;
}

void
Job::setDateFinished(QDateTime const &pDateFinished) {
  auto p          = p_func();
  p->dateFinished = pDateFinished;
  p->modified     = true;
}

void
Job::setQuitAfterFinished(bool pQuitAfterFinished) {
  auto p               = p_func();
  p->quitAfterFinished = pQuitAfterFinished;
  p->modified          = true;
}

void
Job::action(std::function<void()> code) {
  auto p = p_func();

  QMutexLocker locked{&p->mutex};

  code();
}

void
Job::setStatus(Status status) {
  auto p = p_func();

  QMutexLocker locked{&p->mutex};

  if (status == p->status)
    return;

  auto oldStatus = p->status;
  p->status       = status;
  p->modified     = true;

  if (Running == status) {
    p->dateStarted = QDateTime::currentDateTime();
    p->fullOutput.clear();
    p->output.clear();
    p->warnings.clear();
    p->errors.clear();
    p->warningsAcknowledged = 0;
    p->errorsAcknowledged   = 0;

  } else if ((DoneOk == status) || (DoneWarnings == status) || (Failed == status) || (Aborted == status))
    p->dateFinished = QDateTime::currentDateTime();

  if (oldStatus == Running)
    runProgramsAfterCompletion();

  emit statusChanged(p->id, oldStatus, status);
}

bool
Job::isToBeProcessed()
  const {
  return mtx::included_in(p_func()->status, Running, PendingAuto);
}

void
Job::setProgress(unsigned int progress) {
  auto p = p_func();

  QMutexLocker locked{&p->mutex};

  if (progress == p->progress)
    return;

  p->progress = progress;
  p->modified = true;
  emit progressChanged(p->id, p->progress);
}

void
Job::setPendingAuto() {
  auto p = p_func();

  QMutexLocker locked{&p->mutex};

  if (!mtx::included_in(p->status, PendingAuto, Running)) {
    setProgress(0);
    setStatus(PendingAuto);
  }
}

void
Job::setPendingManual() {
  auto p = p_func();

  QMutexLocker locked{&p->mutex};

  if ((PendingManual != p->status) && (Running != p->status)) {
    setProgress(0);
    setStatus(PendingManual);
  }
}

void
Job::addLineToInternalLogs(QString const &line,
                           LineType type) {
  auto p        = p_func();
  auto &storage = InfoLine    == type ? p->output
                : WarningLine == type ? p->warnings
                :                       p->errors;

  auto prefix   = InfoLine    == type ? Q("")
                : WarningLine == type ? Q("%1 ").arg(QY("Warning:"))
                :                       Q("%1 ").arg(QY("Error:"));

  p->fullOutput << Q("%1%2").arg(prefix).arg(line);
  storage       << line;

  p->modified    = true;

  if ((WarningLine == type) || (ErrorLine == type))
    updateUnacknowledgedWarningsAndErrors();
}

QString
Job::displayableStatus(Status status) {
  return PendingManual == status ? QY("Pending manual start")
       : PendingAuto   == status ? QY("Pending automatic start")
       : Running       == status ? QY("Running")
       : DoneOk        == status ? QY("Completed OK")
       : DoneWarnings  == status ? QY("Completed with warnings")
       : Failed        == status ? QY("Failed")
       : Aborted       == status ? QY("Aborted by user")
       : Disabled      == status ? QY("Disabled")
       :                           QY("Unknown");
}

QString
Job::outputFolder()
  const {
  auto destination = destinationFileName();
  return destination.isEmpty() ? QString{} : QFileInfo{destination}.dir().path();
}

void
Job::openOutputFolder()
  const {
  auto folder = outputFolder();

  if (!folder.isEmpty())
    QDesktopServices::openUrl(Util::pathToFileUrl(folder));
}

QString
Job::queueLocation() {
  return Q("%1/%2").arg(Util::Settings::iniFileLocation()).arg("jobQueue");
}

QString
Job::queueFileName()
  const {
  return Q("%1/%2.mtxcfg").arg(queueLocation()).arg(p_func()->uuid.toString());
}

void
Job::removeQueueFile()
  const {
  QFile{queueFileName()}.remove();
}

void
Job::saveQueueFile() {
  auto p = p_func();

  if (p->uuid.isNull())
    p->uuid = QUuid::createUuid();

  auto fileName = queueFileName();
  if (!isModified() && QFileInfo{ fileName }.exists())
    return;

  auto settings = Util::ConfigFile::create(fileName);
  saveJob(*settings);
  settings->save();
}

void
Job::saveJob(Util::ConfigFile &settings) {
  auto p             = p_func();
  auto resetCounters = Util::Settings::get().m_resetJobWarningErrorCountersOnExit;

  settings.setValue("uuid",                 p->uuid);
  settings.setValue("status",               static_cast<unsigned int>(p->status));
  settings.setValue("description",          p->description);
  settings.setValue("output",               p->output);
  settings.setValue("warnings",             p->warnings);
  settings.setValue("errors",               p->errors);
  settings.setValue("fullOutput",           p->fullOutput);
  settings.setValue("progress",             p->progress);
  settings.setValue("exitCode",             p->exitCode);
  settings.setValue("warningsAcknowledged", resetCounters ? p->warnings.count() : p->warningsAcknowledged);
  settings.setValue("errorsAcknowledged",   resetCounters ? p->errors.count()   : p->errorsAcknowledged);
  settings.setValue("dateAdded",            p->dateAdded);
  settings.setValue("dateStarted",          p->dateStarted);
  settings.setValue("dateFinished",         p->dateFinished);

  saveJobInternal(settings);

  p->modified = false;
}

void
Job::loadJobBasis(Util::ConfigFile &settings) {
  auto p                  = p_func();
  p->modified             = false;
  p->uuid                 = settings.value("uuid").toUuid();
  p->status               = static_cast<Status>(settings.value("status", static_cast<unsigned int>(PendingManual)).toUInt());
  p->description          = settings.value("description").toString();
  p->output               = settings.value("output").toStringList();
  p->warnings             = settings.value("warnings").toStringList();
  p->errors               = settings.value("errors").toStringList();
  p->fullOutput           = settings.value("fullOutput").toStringList();
  p->progress             = settings.value("progress").toUInt();
  p->exitCode             = settings.value("exitCode").toUInt();
  p->warningsAcknowledged = settings.value("warningsAcknowledged", 0).toUInt();
  p->errorsAcknowledged   = settings.value("errorsAcknowledged",   0).toUInt();
  p->dateAdded            = settings.value("dateAdded").toDateTime();
  p->dateStarted          = settings.value("dateStarted").toDateTime();
  p->dateFinished         = settings.value("dateFinished").toDateTime();

  if (p->uuid.isNull())
    p->uuid = QUuid::createUuid();

  if (Running == p->status)
    p->status = Aborted;

  if (Util::Settings::get().m_resetJobWarningErrorCountersOnExit) {
    p->warningsAcknowledged = p->warnings.count();
    p->errorsAcknowledged   = p->errors.count();
  }
}

JobPtr
Job::loadJob(QString const &fileName) {
  if (!QFileInfo{fileName}.exists())
    return {};

  auto settings = Util::ConfigFile::open(fileName);
  if (!settings)
    throw Merge::InvalidSettingsX{};

  return loadJob(*settings);
}

JobPtr
Job::loadJob(Util::ConfigFile &settings) {
  auto jobType = settings.value("jobType").toString();

  if (jobType.isEmpty())
    // Older versions didn't write a jobType attribute and only
    // supported MuxJobs.
    jobType = "MuxJob";

  if (jobType == "MuxJob")
    return MuxJob::loadMuxJob(settings);

  if (jobType == "InfoJob")
    return InfoJob::loadInfoJob(settings);

  log_it(boost::format("MTX Job::loadJob: Unknown job type encountered (%1%) in %2%") % jobType % settings.fileName());
  throw Merge::InvalidSettingsX{};
}

void
Job::acknowledgeWarnings() {
  auto p = p_func();

  if (p->warnings.count() == p->warningsAcknowledged)
    return;

  p->warningsAcknowledged = p->warnings.count();
  p->modified             = true;
  updateUnacknowledgedWarningsAndErrors();
}

void
Job::acknowledgeErrors() {
  auto p = p_func();

  if (p->errors.count() == p->errorsAcknowledged)
    return;

  p->errorsAcknowledged = p->errors.count();
  p->modified           = true;
  updateUnacknowledgedWarningsAndErrors();
}

void
Job::updateUnacknowledgedWarningsAndErrors() {
  emit numUnacknowledgedWarningsOrErrorsChanged(p_func()->id, numUnacknowledgedWarnings(), numUnacknowledgedErrors());
}

int
Job::numUnacknowledgedWarnings()
  const {
  auto p = p_func();

  return p->warnings.count() - p->warningsAcknowledged;
}

int
Job::numUnacknowledgedErrors()
  const {
  auto p = p_func();

  return p->errors.count() - p->errorsAcknowledged;
}

void
Job::runProgramsAfterCompletion() {
  auto p = p_func();

  if (!mtx::included_in(p->status, DoneOk, DoneWarnings, Failed))
    return;

  auto event = p->status == Failed ? Util::Settings::RunAfterJobCompletesWithErrors : Util::Settings::RunAfterJobCompletesSuccessfully;

  App::programRunner().run(event, [this](ProgramRunner::VariableMap &variables) {
    runProgramSetupVariables(variables);
  });

  App::programRunner().executeActionsAfterJobFinishes(*this);
}

void
Job::runProgramSetupVariables(ProgramRunner::VariableMap &variables)
  const{
  auto p = p_func();

  variables[Q("JOB_START_TIME")]  << p->dateStarted.toString(Qt::ISODate);
  variables[Q("JOB_END_TIME")]    << p->dateFinished.toString(Qt::ISODate);
  variables[Q("JOB_DESCRIPTION")] << p->description;
  variables[Q("JOB_EXIT_CODE")]   << QString::number(p->exitCode);
}

}}}
