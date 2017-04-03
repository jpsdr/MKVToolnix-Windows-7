#include "common/common_pch.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QSettings>
#include <QUrl>

#include "common/list_utils.h"
#include "common/logger.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/app.h"
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
  : d_ptr{new JobPrivate{status}}
{
  setupJobConnections();
}

Job::Job(JobPrivate &d)
  : d_ptr{&d}
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
  Q_D(const Job);

  return d->id;
}

QUuid
Job::uuid()
  const {
  Q_D(const Job);

  return d->uuid;
}

Job::Status
Job::status()
  const {
  Q_D(const Job);

  return d->status;
}

QString
Job::description()
  const {
  Q_D(const Job);

  return d->description;
}

unsigned int
Job::progress()
  const {
  Q_D(const Job);

  return d->progress;
}

QStringList const &
Job::output()
  const {
  Q_D(const Job);

  return d->output;
}

QStringList const &
Job::warnings()
  const {
  Q_D(const Job);

  return d->warnings;
}

QStringList const &
Job::errors()
  const {
  Q_D(const Job);

  return d->errors;
}

QStringList const &
Job::fullOutput()
  const {
  Q_D(const Job);

  return d->fullOutput;
}

QDateTime
Job::dateAdded()
  const {
  Q_D(const Job);

  return d->dateAdded;
}

QDateTime
Job::dateStarted()
  const {
  Q_D(const Job);

  return d->dateStarted;
}

QDateTime
Job::dateFinished()
  const {
  Q_D(const Job);

  return d->dateFinished;
}

bool
Job::isModified()
  const {
  Q_D(const Job);

  return d->modified;
}

void
Job::setDescription(QString const &pDescription) {
  Q_D(Job);

  d->description = pDescription;
  d->modified    = true;
}

void
Job::setDateAdded(QDateTime const &pDateAdded) {
  Q_D(Job);

  d->dateAdded = pDateAdded;
  d->modified  = true;
}

void
Job::setDateFinished(QDateTime const &pDateFinished) {
  Q_D(Job);

  d->dateFinished = pDateFinished;
  d->modified     = true;
}

void
Job::setQuitAfterFinished(bool pQuitAfterFinished) {
  Q_D(Job);

  d->quitAfterFinished = pQuitAfterFinished;
  d->modified          = true;
}

void
Job::action(std::function<void()> code) {
  Q_D(Job);

  QMutexLocker locked{&d->mutex};

  code();
}

void
Job::setStatus(Status status) {
  Q_D(Job);

  QMutexLocker locked{&d->mutex};

  if (status == d->status)
    return;

  auto oldStatus = d->status;
  d->status       = status;
  d->modified     = true;

  if (Running == status) {
    d->dateStarted = QDateTime::currentDateTime();
    d->fullOutput.clear();
    d->output.clear();
    d->warnings.clear();
    d->errors.clear();
    d->warningsAcknowledged = 0;
    d->errorsAcknowledged   = 0;

  } else if ((DoneOk == status) || (DoneWarnings == status) || (Failed == status) || (Aborted == status))
    d->dateFinished = QDateTime::currentDateTime();

  if (oldStatus == Running)
    runProgramsAfterCompletion();

  emit statusChanged(d->id, oldStatus, status);
}

bool
Job::isToBeProcessed()
  const {
  Q_D(const Job);

  return (Running == d->status) || (PendingAuto == d->status);
}

void
Job::setProgress(unsigned int progress) {
  Q_D(Job);

  QMutexLocker locked{&d->mutex};

  if (progress == d->progress)
    return;

  d->progress = progress;
  d->modified = true;
  emit progressChanged(d->id, d->progress);
}

void
Job::setPendingAuto() {
  Q_D(Job);

  QMutexLocker locked{&d->mutex};

  if ((PendingAuto != d->status) && (Running != d->status))
    setStatus(PendingAuto);
}

void
Job::setPendingManual() {
  Q_D(Job);

  QMutexLocker locked{&d->mutex};

  if ((PendingManual != d->status) && (Running != d->status))
    setStatus(PendingManual);
}

void
Job::addLineToInternalLogs(QString const &line,
                           LineType type) {
  Q_D(Job);

  auto &storage = InfoLine    == type ? d->output
                : WarningLine == type ? d->warnings
                :                       d->errors;

  auto prefix   = InfoLine    == type ? Q("")
                : WarningLine == type ? Q("%1 ").arg(QY("Warning:"))
                :                       Q("%1 ").arg(QY("Error:"));

  d->fullOutput << Q("%1%2").arg(prefix).arg(line);
  storage       << line;

  d->modified    = true;

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
  return {};
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
  Q_D(const Job);

  return Q("%1/%2.mtxcfg").arg(queueLocation()).arg(d->uuid.toString());
}

void
Job::removeQueueFile()
  const {
  QFile{queueFileName()}.remove();
}

void
Job::saveQueueFile() {
  Q_D(Job);

  if (d->uuid.isNull())
    d->uuid = QUuid::createUuid();

  auto fileName = queueFileName();
  if (!isModified() && QFileInfo{ fileName }.exists())
    return;

  auto settings = Util::ConfigFile::create(fileName);
  saveJob(*settings);
  settings->save();
}

void
Job::saveJob(Util::ConfigFile &settings) {
  Q_D(Job);

  auto resetCounters = Util::Settings::get().m_resetJobWarningErrorCountersOnExit;

  settings.setValue("uuid",                 d->uuid);
  settings.setValue("status",               static_cast<unsigned int>(d->status));
  settings.setValue("description",          d->description);
  settings.setValue("output",               d->output);
  settings.setValue("warnings",             d->warnings);
  settings.setValue("errors",               d->errors);
  settings.setValue("fullOutput",           d->fullOutput);
  settings.setValue("progress",             d->progress);
  settings.setValue("exitCode",             d->exitCode);
  settings.setValue("warningsAcknowledged", resetCounters ? d->warnings.count() : d->warningsAcknowledged);
  settings.setValue("errorsAcknowledged",   resetCounters ? d->errors.count()   : d->errorsAcknowledged);
  settings.setValue("dateAdded",            d->dateAdded);
  settings.setValue("dateStarted",          d->dateStarted);
  settings.setValue("dateFinished",         d->dateFinished);

  saveJobInternal(settings);

  d->modified = false;
}

void
Job::loadJobBasis(Util::ConfigFile &settings) {
  Q_D(Job);

  d->modified             = false;
  d->uuid                 = settings.value("uuid").toUuid();
  d->status               = static_cast<Status>(settings.value("status", static_cast<unsigned int>(PendingManual)).toUInt());
  d->description          = settings.value("description").toString();
  d->output               = settings.value("output").toStringList();
  d->warnings             = settings.value("warnings").toStringList();
  d->errors               = settings.value("errors").toStringList();
  d->fullOutput           = settings.value("fullOutput").toStringList();
  d->progress             = settings.value("progress").toUInt();
  d->exitCode             = settings.value("exitCode").toUInt();
  d->warningsAcknowledged = settings.value("warningsAcknowledged", 0).toUInt();
  d->errorsAcknowledged   = settings.value("errorsAcknowledged",   0).toUInt();
  d->dateAdded            = settings.value("dateAdded").toDateTime();
  d->dateStarted          = settings.value("dateStarted").toDateTime();
  d->dateFinished         = settings.value("dateFinished").toDateTime();

  if (d->uuid.isNull())
    d->uuid = QUuid::createUuid();

  if (Running == d->status)
    d->status = Aborted;

  if (Util::Settings::get().m_resetJobWarningErrorCountersOnExit) {
    d->warningsAcknowledged = d->warnings.count();
    d->errorsAcknowledged   = d->errors.count();
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

  log_it(boost::format("MTX Job::loadJob: Unknown job type encountered (%1%) in %2%") % jobType % settings.fileName());
  throw Merge::InvalidSettingsX{};
}

void
Job::acknowledgeWarnings() {
  Q_D(Job);

  if (d->warnings.count() == d->warningsAcknowledged)
    return;

  d->warningsAcknowledged = d->warnings.count();
  d->modified             = true;
  updateUnacknowledgedWarningsAndErrors();
}

void
Job::acknowledgeErrors() {
  Q_D(Job);

  if (d->errors.count() == d->errorsAcknowledged)
    return;

  d->errorsAcknowledged = d->errors.count();
  d->modified           = true;
  updateUnacknowledgedWarningsAndErrors();
}

void
Job::updateUnacknowledgedWarningsAndErrors() {
  Q_D(Job);

  emit numUnacknowledgedWarningsOrErrorsChanged(d->id, numUnacknowledgedWarnings(), numUnacknowledgedErrors());
}

int
Job::numUnacknowledgedWarnings()
  const {
  Q_D(const Job);

  return d->warnings.count() - d->warningsAcknowledged;
}

int
Job::numUnacknowledgedErrors()
  const {
  Q_D(const Job);

  return d->errors.count() - d->errorsAcknowledged;
}

void
Job::runProgramsAfterCompletion() {
  Q_D(Job);

  if (!mtx::included_in(d->status, DoneOk, DoneWarnings, Failed))
    return;

  auto event = d->status == Failed ? Util::Settings::RunAfterJobCompletesWithErrors : Util::Settings::RunAfterJobCompletesSuccessfully;

  App::programRunner().run(event, [this](ProgramRunner::VariableMap &variables) {
    runProgramSetupVariables(variables);
  });

  App::programRunner().executeActionsAfterJobFinishes(*this);
}

void
Job::runProgramSetupVariables(ProgramRunner::VariableMap &variables)
  const{
  Q_D(const Job);

  variables[Q("JOB_START_TIME")]  << d->dateStarted.toString(Qt::ISODate);
  variables[Q("JOB_END_TIME")]    << d->dateFinished.toString(Qt::ISODate);
  variables[Q("JOB_DESCRIPTION")] << d->description;
  variables[Q("JOB_EXIT_CODE")]   << QString::number(d->exitCode);
}

}}}
