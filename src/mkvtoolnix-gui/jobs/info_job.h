#pragma once

#include "common/common_pch.h"

#include "common/qt.h"
#include "mkvtoolnix-gui/jobs/job.h"
#include "mkvtoolnix-gui/util/kax_info.h"

namespace mtx { namespace gui {

namespace Info {
class InfoConfig;
using InfoConfigPtr = std::shared_ptr<InfoConfig>;
}

namespace Util {
class ConfigFile;
}

namespace Jobs {

class InfoJobPrivate;
class InfoJob: public Job {
  Q_OBJECT

protected:
  MTX_DECLARE_PRIVATE(InfoJobPrivate)

  explicit InfoJob(InfoJobPrivate &p);

public:
  InfoJob(Status status, mtx::gui::Info::InfoConfigPtr const &config);
  virtual ~InfoJob();

  virtual void start();

  virtual QString destinationFileName() const override;
  virtual QString displayableType() const override;
  virtual QString displayableDescription() const override;
  virtual bool isEditable() const override;

  virtual Info::InfoConfig const &config() const;

public slots:
  virtual void abort() override;
  virtual void showError(const QString &message);
  virtual void updateProgress(int percentage, const QString &text);
  virtual void infoStarted();
  virtual void infoFinished(mtx::kax_info_c::result_e result);

protected:
  virtual void setupInfoJobConnections();
  virtual void saveJobInternal(Util::ConfigFile &settings) const;

public:
  static JobPtr loadInfoJob(Util::ConfigFile &settings);
};

}}}
