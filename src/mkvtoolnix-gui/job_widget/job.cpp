#include "common/common_pch.h"

#include <QSettings>

#include "common/qt.h"
#include "mkvtoolnix-gui/job_widget/job.h"
#include "mkvtoolnix-gui/job_widget/mux_job.h"

uint64_t Job::ms_next_id = 0;

Job::Job(Status status)
  : m_id{ms_next_id++}
  , m_status{status}
  , m_progress{}
  , m_exitCode{std::numeric_limits<unsigned int>::max()}
  , m_mutex{QMutex::Recursive}
{
  connect(this, SIGNAL(lineRead(const QString&,Job::LineType)), this, SLOT(addLineToInternalLogs(const QString&,Job::LineType)));
}

Job::~Job() {
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

  } else if ((DoneOk == status) || (DoneWarnings == status) || (Failed == status) || (Aborted == status))
    m_dateFinished = QDateTime::currentDateTime();

  emit statusChanged(m_id, m_status);
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
Job::addLineToInternalLogs(QString const &line,
                           LineType type) {
  auto &storage = InfoLine    == type ? m_output
                : WarningLine == type ? m_warnings
                :                       m_errors;

  auto prefix   = InfoLine    == type ? Q("")
                : WarningLine == type ? QY("Warning: ")
                :                       QY("Error: ");

  m_fullOutput << Q("%1%2\n").arg(prefix).arg(line);
  storage      << line;
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

void
Job::saveJob(QSettings &settings)
  const {

  settings.setValue("status",       static_cast<unsigned int>(m_status));
  settings.setValue("description",  m_description);
  settings.setValue("output",       m_output);
  settings.setValue("warnings",     m_warnings);
  settings.setValue("errors",       m_errors);
  settings.setValue("fullOutput",   m_fullOutput);
  settings.setValue("progress",     m_progress);
  settings.setValue("exitCode",     m_exitCode);
  settings.setValue("dateAdded",    m_dateAdded);
  settings.setValue("dateStarted",  m_dateStarted);
  settings.setValue("dateFinished", m_dateFinished);

  saveJobInternal(settings);
}

void
Job::loadJobBasis(QSettings &settings) {
  m_status       = static_cast<Status>(settings.value("status", static_cast<unsigned int>(PendingManual)).toUInt());
  m_description  = settings.value("description").toString();
  m_output       = settings.value("output").toStringList();
  m_warnings     = settings.value("warnings").toStringList();
  m_errors       = settings.value("errors").toStringList();
  m_fullOutput   = settings.value("fullOutput").toStringList();
  m_progress     = settings.value("progress").toUInt();
  m_exitCode     = settings.value("exitCode").toUInt();
  m_dateAdded    = settings.value("dateAdded").toDateTime();
  m_dateStarted  = settings.value("dateStarted").toDateTime();
  m_dateFinished = settings.value("dateFinished").toDateTime();
}

JobPtr
Job::loadJob(QSettings &settings) {
  auto jobType = settings.value("jobType");

  if (jobType == "MuxJob")
    return MuxJob::loadMuxJob(settings);

  Q_ASSERT_X(false, "Job::loadJob", "Unknown job type encountered");

  return JobPtr{};
}
