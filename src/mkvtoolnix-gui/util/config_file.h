#pragma once

#include "common/common_pch.h"

#include <QVariant>

namespace mtx { namespace gui { namespace Util {

class ConfigFile;
using ConfigFilePtr = std::shared_ptr<ConfigFile>;

class ConfigFile: public QObject {
  Q_OBJECT;
public:
  enum Type {
    UnknownType,
    Ini,
    Json,
  };

protected:
  QString m_fileName;

public:
  ConfigFile(QString const &fileName);
  virtual ~ConfigFile();

  QString const &fileName() const;
  void setFileName(QString const &name);

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

  static Type determineType(QString const &fileName);

private:
  static ConfigFilePtr openInternal(QString const &fileName);
};

}}}
