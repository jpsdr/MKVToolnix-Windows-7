#ifndef MTX_MKVTOOLNIX_GUI_UTIL_CONFIG_FILE_H
#define MTX_MKVTOOLNIX_GUI_UTIL_CONFIG_FILE_H

#include "common/common_pch.h"

#include <QVariant>

namespace mtx { namespace gui { namespace Util {

class ConfigFile;
using ConfigFilePtr = std::shared_ptr<ConfigFile>;

class ConfigFile: public QObject {
  Q_OBJECT;
protected:
  QString m_fileName;

public:
  ConfigFile(QString const &fileName);
  virtual ~ConfigFile();

  virtual void load() = 0;
  virtual void save() = 0;

  virtual void beginGroup(QString const &group) = 0;
  virtual void endGroup() = 0;

  virtual void setValue(QString const &key, QVariant const &value) = 0;
  virtual QVariant value(QString const &key, QVariant const &defaultValue = QVariant{}) const = 0;

  virtual QStringList childGroups() = 0;
  virtual QStringList childKeys() = 0;

public:
  static ConfigFilePtr open(QString const &fileName);
  static ConfigFilePtr create(QString const &fileName);

private:
  static ConfigFilePtr openInternal(QString const &fileName);
};

}}}

#endif  // MTX_MKVTOOLNIX_GUI_UTIL_CONFIG_FILE_H
