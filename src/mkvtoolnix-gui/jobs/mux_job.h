#pragma once

#include "common/common_pch.h"

#include <QByteArray>
#include <QProcess>

#include "common/qt.h"
#include "mkvtoolnix-gui/jobs/job.h"

namespace mtx::gui {

namespace Merge {
class MuxConfig;
using MuxConfigPtr = std::shared_ptr<MuxConfig>;
}

namespace Util {
class ConfigFile;
}

namespace Jobs {

class MuxJobPrivate;
class MuxJob: public Job {
  Q_OBJECT

protected:
  MTX_DECLARE_PRIVATE(MuxJobPrivate)

  explicit MuxJob(MuxJobPrivate &p);

public:
  MuxJob(Status status, mtx::gui::Merge::MuxConfigPtr const &config);
  virtual ~MuxJob();

  virtual void start();

  virtual QString destinationFileName() const override;
  virtual QString displayableType() const override;
  virtual QString displayableDescription() const override;
  virtual bool isEditable() const override;

  virtual Merge::MuxConfig const &config() const;

  virtual void runProgramSetupVariables(ProgramRunner::VariableMap &variables) const override;

public Q_SLOTS:
  virtual void readAvailable();
  virtual void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
  virtual void processError(QProcess::ProcessError error);
  virtual void abort() override;

protected:
  void setupMuxJobConnections();
  void processBytesRead();
  void processLine(QString const &rawLine);
  virtual void saveJobInternal(Util::ConfigFile &settings) const;

Q_SIGNALS:
  void startedScanningPlaylists();
  void finishedScanningPlaylists();

public:
  static JobPtr loadMuxJob(Util::ConfigFile &settings);
};

}}
