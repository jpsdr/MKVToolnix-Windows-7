#include "common/common_pch.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QSettings>
#include <QUrl>

#include "common/qt.h"
#include "mkvtoolnix-gui/jobs/job.h"
#include "mkvtoolnix-gui/jobs/mux_job.h"
#include "mkvtoolnix-gui/util/config_file.h"
#include "mkvtoolnix-gui/util/settings.h"

namespace mtx { namespace gui { namespace Jobs {

uint64_t Job::ms_next_id = 0;

Job::Job(Status status)
  : m_id{ms_next_id++}
  , m_status{status}
  , m_progress{}
  , m_exitCode{std::numeric_limits<unsigned int>::max()}
  , m_warningsAcknowledged{}
  , m_errorsAcknowledged{}
  , m_quitAfterFinished{}
  , m_mutex{QMutex::Recursive}
{
  connect(this, &Job::lineRead, this, &Job::addLineToInternalLogs);
}

Job::~Job() {
}

void
Job::action(std::function<void()> code) {
  QMutexLocker locked{&m_mutex};

  code();
}

void
Job::setStatus(Status status) {
  QMutexLocker locked{&m_mutex};

  if (status == m_status)
    return;

  m_status = status;

  if (Running == status) {
    m_dateStarted = QDateTime::currentDateTime();
    m_fullOutput.clear();
    m_output.clear();
    m_warnings.clear();
    m_errors.clear();
    m_warningsAcknowledged = 0;
    m_errorsAcknowledged   = 0;

  } else if ((DoneOk == status) || (DoneWarnings == status) || (Failed == status) || (Aborted == status))
    m_dateFinished = QDateTime::currentDateTime();

  emit statusChanged(m_id, status);
}

bool
Job::isToBeProcessed()
  const {
  return (Running == m_status) || (PendingAuto == m_status);
}

void
Job::setProgress(unsigned int progress) {
  QMutexLocker locked{&m_mutex};

  if (progress == m_progress)
    return;

  m_progress = progress;
  emit progressChanged(m_id, m_progress);
}

void
Job::setPendingAuto() {
  QMutexLocker locked{&m_mutex};

  if ((PendingAuto != m_status) && (Running != m_status))
    setStatus(PendingAuto);
}

void
Job::setPendingManual() {
  QMutexLocker locked{&m_mutex};

  if ((PendingManual != m_status) && (Running != m_status))
    setStatus(PendingManual);
}

void
Job::addLineToInternalLogs(QString const &line,
                           LineType type) {
  auto &storage = InfoLine    == type ? m_output
                : WarningLine == type ? m_warnings
                :                       m_errors;

  auto prefix   = InfoLine    == type ? Q("")
                : WarningLine == type ? Q("%1 ").arg(QY("Warning:"))
                :                       Q("%1 ").arg(QY("Error:"));

  m_fullOutput << Q("%1%2").arg(prefix).arg(line);
  storage      << line;

  if ((WarningLine == type) || (ErrorLine == type))
    updateUnacknowledgedWarningsAndErrors();
}

QString
Job::displayableStatus(Status status) {
  return PendingManual == status ? QY("pending manual start")
       : PendingAuto   == status ? QY("pending automatic start")
       : Running       == status ? QY("running")
       : DoneOk        == status ? QY("completed OK")
       : DoneWarnings  == status ? QY("completed with warnings")
       : Failed        == status ? QY("failed")
       : Aborted       == status ? QY("aborted by user")
       : Disabled      == status ? QY("disabled")
       :                           QY("unknown");
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
    QDesktopServices::openUrl(QUrl{Q("file:///%1").arg(folder)});
}

QString
Job::queueLocation() {
  return Q("%1/%2").arg(Util::Settings::iniFileLocation()).arg("jobQueue");
}

QString
Job::queueFileName()
  const {
  return Q("%1/%2.ini").arg(queueLocation()).arg(m_uuid.toString());
}

void
Job::removeQueueFile()
  const {
  QFile{queueFileName()}.remove();
}

void
Job::saveQueueFile() {
  if (m_uuid.isNull())
    m_uuid = QUuid::createUuid();

  auto settings = Util::ConfigFile::create(queueFileName());
  saveJob(*settings);
}

void
Job::saveJob(Util::ConfigFile &settings)
  const {

  settings.setValue("uuid",                 m_uuid);
  settings.setValue("status",               static_cast<unsigned int>(m_status));
  settings.setValue("description",          m_description);
  settings.setValue("output",               m_output);
  settings.setValue("warnings",             m_warnings);
  settings.setValue("errors",               m_errors);
  settings.setValue("fullOutput",           m_fullOutput);
  settings.setValue("progress",             m_progress);
  settings.setValue("exitCode",             m_exitCode);
  settings.setValue("warningsAcknowledged", m_warningsAcknowledged);
  settings.setValue("errorsAcknowledged",   m_errorsAcknowledged);
  settings.setValue("dateAdded",            m_dateAdded);
  settings.setValue("dateStarted",          m_dateStarted);
  settings.setValue("dateFinished",         m_dateFinished);

  saveJobInternal(settings);
}

void
Job::loadJobBasis(Util::ConfigFile &settings) {
  m_uuid                 = settings.value("uuid").toUuid();
  m_status               = static_cast<Status>(settings.value("status", static_cast<unsigned int>(PendingManual)).toUInt());
  m_description          = settings.value("description").toString();
  m_output               = settings.value("output").toStringList();
  m_warnings             = settings.value("warnings").toStringList();
  m_errors               = settings.value("errors").toStringList();
  m_fullOutput           = settings.value("fullOutput").toStringList();
  m_progress             = settings.value("progress").toUInt();
  m_exitCode             = settings.value("exitCode").toUInt();
  m_warningsAcknowledged = settings.value("warningsAcknowledged", 0).toUInt();
  m_errorsAcknowledged   = settings.value("errorsAcknowledged",   0).toUInt();
  m_dateAdded            = settings.value("dateAdded").toDateTime();
  m_dateStarted          = settings.value("dateStarted").toDateTime();
  m_dateFinished         = settings.value("dateFinished").toDateTime();

  if (m_uuid.isNull())
    m_uuid = QUuid::createUuid();

  if (Running == m_status)
    m_status = Aborted;
}

JobPtr
Job::loadJob(QString const &fileName) {
  if (!QFileInfo{fileName}.exists())
    return {};

  auto settings = Util::ConfigFile::open(fileName);
  return loadJob(*settings);
}

JobPtr
Job::loadJob(Util::ConfigFile &settings) {
  auto jobType = settings.value("jobType");

  if (jobType == "MuxJob")
    return MuxJob::loadMuxJob(settings);

  Q_ASSERT_X(false, "Job::loadJob", "Unknown job type encountered");

  return JobPtr{};
}

void
Job::acknowledgeWarnings() {
  m_warningsAcknowledged = m_warnings.count();
  updateUnacknowledgedWarningsAndErrors();
}

void
Job::acknowledgeErrors() {
  m_errorsAcknowledged = m_errors.count();
  updateUnacknowledgedWarningsAndErrors();
}

void
Job::updateUnacknowledgedWarningsAndErrors() {
  emit numUnacknowledgedWarningsOrErrorsChanged(m_id, numUnacknowledgedWarnings(), numUnacknowledgedErrors());
}

int
Job::numUnacknowledgedWarnings()
  const {
  return m_warnings.count() - m_warningsAcknowledged;
}

int
Job::numUnacknowledgedErrors()
  const {
  return m_errors.count() - m_errorsAcknowledged;
}

}}}
