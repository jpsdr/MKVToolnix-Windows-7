#pragma once

#include "common/common_pch.h"

#include "mkvtoolnix-gui/util/config_file.h"

namespace mtx { namespace gui { namespace Util {

class IniConfigFile: public ConfigFile {
protected:
  QSettings *m_settings;
  bool m_settingsAreOwned;

public:
  IniConfigFile(QString const &fileName);
  IniConfigFile(QSettings &settings);
  virtual ~IniConfigFile();

  virtual void load() override;
  virtual void save() override;

  virtual void beginGroup(QString const &group) override;
  virtual void endGroup() override;

  virtual void setValue(QString const &key, QVariant const &value) override;
  virtual QVariant value(QString const &key, QVariant const &defaultValue = QVariant{}) const override;

  virtual QStringList childGroups() override;
  virtual QStringList childKeys() override;
};

}}}
