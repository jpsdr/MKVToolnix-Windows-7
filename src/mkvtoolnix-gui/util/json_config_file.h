#pragma once

#include "common/common_pch.h"

#include <QStack>

#include "mkvtoolnix-gui/util/config_file.h"

namespace mtx { namespace gui { namespace Util {

namespace Config {

};

class JsonConfigFile: public ConfigFile {
public:
  class Group;
  using GroupPtr = std::shared_ptr<Group>;

  struct Group {
    QHash<QString, GroupPtr> m_groups;
    QHash<QString, QVariant> m_data;
  };

protected:
  QStack<Group *> m_currentGroup;
  GroupPtr m_rootGroup;

public:
  JsonConfigFile(QString const &fileName);
  virtual ~JsonConfigFile();

  virtual void load() override;
  virtual void save() override;

  virtual void beginGroup(QString const &name) override;
  virtual void endGroup() override;

  virtual void setValue(QString const &key, QVariant const &value) override;
  virtual QVariant value(QString const &key, QVariant const &defaultValue = QVariant{}) const override;

  virtual QStringList childGroups() override;
  virtual QStringList childKeys() override;

protected:
  void reset(GroupPtr const &newGroup = GroupPtr{});
};

}}}
