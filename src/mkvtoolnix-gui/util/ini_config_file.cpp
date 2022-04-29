#include "common/common_pch.h"

#include <QSettings>

#include "mkvtoolnix-gui/util/ini_config_file.h"

namespace mtx::gui::Util {

IniConfigFile::IniConfigFile(QString const &fileName)
  : ConfigFile{fileName}
  , m_settings{new QSettings{fileName, QSettings::IniFormat}}
  , m_settingsAreOwned{true}
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  m_settings->setIniCodec("UTF-8");
#endif
}

IniConfigFile::IniConfigFile(QSettings &settings)
  : ConfigFile{QString{}}
  , m_settings{&settings}
  , m_settingsAreOwned{false}
{
}

IniConfigFile::~IniConfigFile() {
  if (m_settingsAreOwned)
    delete m_settings;
}

void
IniConfigFile::load() {
}

void
IniConfigFile::save() {
  m_settings->sync();
}

void
IniConfigFile::beginGroup(QString const &group) {
  m_settings->beginGroup(group);
}

void
IniConfigFile::endGroup() {
  m_settings->endGroup();
}

void
IniConfigFile::remove(QString const &key) {
  m_settings->remove(key);
}

void
IniConfigFile::setValue(QString const &key,
                        QVariant const &value) {
  m_settings->setValue(key, value);
}

QVariant
IniConfigFile::value(QString const &key,
                     QVariant const &defaultValue)
  const {
  return m_settings->value(key, defaultValue);
}

QStringList
IniConfigFile::childGroups() {
  return m_settings->childGroups();
}

QStringList
IniConfigFile::childKeys() {
  return m_settings->childKeys();
}

}
