#ifndef MTX_MKVTOOLNIX_GUI_JOBS_JOB_H
#define MTX_MKVTOOLNIX_GUI_JOBS_JOB_H

#include "common/common_pch.h"

#include <QDateTime>
#include <QObject>
#include <QMutex>
#include <QString>
#include <QStringList>
#include <QUuid>

class QSettings;

namespace mtx { namespace gui { namespace Jobs {

class Job;
using JobPtr = std::shared_ptr<Job>;

class Job: public QObject {
  Q_OBJECT;
  Q_ENUMS(Status);

public:
  enum Status {
    PendingManual,
    PendingAuto,
    Running,
    DoneOk,
    DoneWarnings,
    Failed,
    Aborted,
    Disabled,
  };

  enum LineType {
    InfoLine,
    WarningLine,
    ErrorLine,
  };

private:
  static uint64_t ms_next_id;

public:
  QUuid m_uuid;
  uint64_t m_id;
  Status m_status;
  QString m_description;
  QStringList m_output, m_warnings, m_errors, m_fullOutput;
  unsigned int m_progress, m_exitCode, m_warningsAcknowledged, m_errorsAcknowledged;
  QDateTime m_dateAdded, m_dateStarted, m_dateFinished;
  bool m_quitAfterFinished;

  QMutex m_mutex;

public:
  Job(Status status = PendingManual);
  virtual ~Job();

  void action(std::function<void()> code);
  bool isToBeProcessed() const;

  virtual void start() = 0;

  virtual QString displayableType() const = 0;
  virtual QString displayableDescription() const = 0;

  void setPendingAuto();
  void setPendingManual();

  QString queueFileName() const;
  void removeQueueFile() const;
  void saveQueueFile();
  void saveJob(QSettings &settings) const;

  void acknowledgeWarnings();
  void acknowledgeErrors();

  int numUnacknowledgedWarnings() const;
  int numUnacknowledgedErrors() const;

protected:
  virtual void saveJobInternal(QSettings &settings) const = 0;
  virtual void loadJobBasis(QSettings &settings);

public slots:
  virtual void setStatus(Job::Status status);
  virtual void setProgress(unsigned int progress);
  virtual void addLineToInternalLogs(QString const &line, mtx::gui::Jobs::Job::LineType type);
  virtual void abort() = 0;
  virtual void updateUnacknowledgedWarningsAndErrors();

signals:
  void statusChanged(uint64_t id);
  void progressChanged(uint64_t id, unsigned int progress);
  void numUnacknowledgedWarningsOrErrorsChanged(uint64_t id, int numWarnings, int numErrors);

  void lineRead(QString const &line, mtx::gui::Jobs::Job::LineType type);

public:                         // static
  static QString displayableStatus(Status status);
  static JobPtr loadJob(QSettings &settings);
  static JobPtr loadJob(QString const &fileName);

  static QString queueLocation();
};

}}}

Q_DECLARE_METATYPE(mtx::gui::Jobs::Job::LineType);
Q_DECLARE_METATYPE(mtx::gui::Jobs::Job::Status);

#endif  // MTX_MKVTOOLNIX_GUI_JOBS_JOB_H
