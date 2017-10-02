#pragma once

#include "common/common_pch.h"

#include <QDateTime>
#include <QObject>
#include <QMutex>
#include <QString>
#include <QStringList>
#include <QUuid>

#include "mkvtoolnix-gui/jobs/program_runner.h"

namespace mtx { namespace gui {

namespace Util {
class ConfigFile;
}

namespace Jobs {

class Job;
using JobPtr = std::shared_ptr<Job>;

class JobPrivate;
class Job: public QObject {
  Q_OBJECT;

protected:
  Q_DECLARE_PRIVATE(Job);

  QScopedPointer<JobPrivate> const d_ptr;

  explicit Job(JobPrivate &d);

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

public:
  Job(Status status = PendingManual);
  virtual ~Job();

  bool isModified() const;

  uint64_t id() const;
  QUuid uuid() const;
  Status status() const;
  QString description() const;
  unsigned int progress() const;

  QStringList const &output() const;
  QStringList const &warnings() const;
  QStringList const &errors() const;
  QStringList const &fullOutput() const;

  QDateTime dateAdded() const;
  QDateTime dateStarted() const;
  QDateTime dateFinished() const;

  void setDescription(QString const &pDescription);
  void setDateAdded(QDateTime const &pDateAdded);
  void setDateFinished(QDateTime const &pDateFinished);
  void setQuitAfterFinished(bool pQuitAfterFinished);

  void action(std::function<void()> code);
  bool isToBeProcessed() const;

  virtual void start() = 0;

  virtual QString displayableType() const = 0;
  virtual QString displayableDescription() const = 0;
  virtual QString outputFolder() const;

  void setPendingAuto();
  void setPendingManual();

  QString queueFileName() const;
  void removeQueueFile() const;
  void saveQueueFile();
  void saveJob(Util::ConfigFile &settings);

  void acknowledgeWarnings();
  void acknowledgeErrors();

  int numUnacknowledgedWarnings() const;
  int numUnacknowledgedErrors() const;

  virtual void runProgramSetupVariables(ProgramRunner::VariableMap &variables) const;

protected:
  virtual void saveJobInternal(Util::ConfigFile &settings) const = 0;
  virtual void loadJobBasis(Util::ConfigFile &settings);
  virtual void runProgramsAfterCompletion();
  void setupJobConnections();

public slots:
  virtual void setStatus(Job::Status status);
  virtual void setProgress(unsigned int progress);
  virtual void addLineToInternalLogs(QString const &line, mtx::gui::Jobs::Job::LineType type);
  virtual void abort() = 0;
  virtual void updateUnacknowledgedWarningsAndErrors();
  virtual void openOutputFolder() const;

signals:
  void statusChanged(uint64_t id, mtx::gui::Jobs::Job::Status oldStatus, mtx::gui::Jobs::Job::Status newStatus);
  void progressChanged(uint64_t id, unsigned int progress);
  void numUnacknowledgedWarningsOrErrorsChanged(uint64_t id, int numWarnings, int numErrors);

  void lineRead(QString const &line, mtx::gui::Jobs::Job::LineType type);

public:                         // static
  static QString displayableStatus(Status status);
  static JobPtr loadJob(Util::ConfigFile &settings);
  static JobPtr loadJob(QString const &fileName);

  static QString queueLocation();
};

}}}

Q_DECLARE_METATYPE(mtx::gui::Jobs::Job::LineType);
Q_DECLARE_METATYPE(mtx::gui::Jobs::Job::Status);
