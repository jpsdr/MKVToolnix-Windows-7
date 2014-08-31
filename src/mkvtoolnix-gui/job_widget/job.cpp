#include "common/common_pch.h"

#include "common/qt.h"
#include "mkvtoolnix-gui/job_widget/job.h"

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
