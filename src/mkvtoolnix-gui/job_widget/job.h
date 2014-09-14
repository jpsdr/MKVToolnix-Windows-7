#ifndef MTX_MKVTOOLNIX_GUI_JOB_H
#define MTX_MKVTOOLNIX_GUI_JOB_H

#include "common/common_pch.h"

#include <QDateTime>
#include <QObject>
#include <QMutex>
#include <QString>
#include <QStringList>

class QSettings;
class Job;
typedef std::shared_ptr<Job> JobPtr;

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
  uint64_t m_id;
  Status m_status;
  QString m_description;
  QStringList m_output, m_warnings, m_errors, m_fullOutput;
  unsigned int m_progress, m_exitCode;
  QDateTime m_dateAdded, m_dateStarted, m_dateFinished;

  QMutex m_mutex;

public:
  Job(Status status = PendingManual);
  virtual ~Job();

  void action(std::function<void()> code);
  bool isToBeProcessed() const;

  virtual void abort() = 0;
  virtual void start() = 0;

  virtual QString displayableType() const = 0;
  virtual QString displayableDescription() const = 0;

  void setPendingAuto();

  void saveJob(QSettings &settings) const;

protected:
  virtual void saveJobInternal(QSettings &settings) const = 0;
  virtual void loadJobBasis(QSettings &settings);

public slots:
  void setStatus(Job::Status status);
  void setProgress(unsigned int progress);
  void addLineToInternalLogs(QString const &line, LineType type);

signals:
  void statusChanged(uint64_t id, Job::Status status);
  void progressChanged(uint64_t id, unsigned int progress);

  void lineRead(QString const &line, Job::LineType type);

public:                         // static
  static QString displayableStatus(Status status);
  static JobPtr loadJob(QSettings &settings);
};

#endif  // MTX_MKVTOOLNIX_GUI_JOB_H
