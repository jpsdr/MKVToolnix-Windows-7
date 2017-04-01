#ifndef MTX_MKVTOOLNIX_GUI_MUX_JOB_H
#define MTX_MKVTOOLNIX_GUI_MUX_JOB_H

#include "common/common_pch.h"

#include <QByteArray>
#include <QProcess>

#include "mkvtoolnix-gui/jobs/job.h"

class QTemporaryFile;

namespace mtx { namespace gui {

namespace Merge {

class MuxConfig;
using MuxConfigPtr = std::shared_ptr<MuxConfig>;

}

namespace Util {
class ConfigFile;
}

namespace Jobs {

class MuxJob: public Job {
  Q_OBJECT;
protected:
  mtx::gui::Merge::MuxConfigPtr m_config;
  QProcess m_process;
  bool m_aborted;
  QByteArray m_bytesRead;
  std::unique_ptr<QTemporaryFile> m_settingsFile;

public:
  MuxJob(Status status, mtx::gui::Merge::MuxConfigPtr const &config);
  virtual ~MuxJob();

  virtual void start();

  virtual QString displayableType() const override;
  virtual QString displayableDescription() const override;
  virtual QString outputFolder() const override;

  virtual Merge::MuxConfig const &config() const;

  virtual void runProgramSetupVariables(ProgramRunner::VariableMap &variables) const override;

public slots:
  virtual void readAvailable();
  virtual void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
  virtual void processError(QProcess::ProcessError error);
  virtual void abort() override;

protected:
  void processBytesRead();
  void processLine(QString const &rawLine);
  virtual void saveJobInternal(Util::ConfigFile &settings) const;

signals:
  void startedScanningPlaylists();
  void finishedScanningPlaylists();

public:
  static JobPtr loadMuxJob(Util::ConfigFile &settings);
};

}}}

#endif // MTX_MKVTOOLNIX_GUI_MUX_JOB_H
