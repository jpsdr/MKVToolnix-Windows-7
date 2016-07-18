#include "common/common_pch.h"

#include <QFile>
#include <QFileInfo>
#include <QSettings>

#include "mkvtoolnix-gui/util/config_file.h"
#include "mkvtoolnix-gui/util/ini_config_file.h"
#include "mkvtoolnix-gui/util/json_config_file.h"

namespace mtx { namespace gui { namespace Util {

ConfigFile::ConfigFile(QString const &fileName)
  : m_fileName{fileName}
{
}

ConfigFile::~ConfigFile() {
}

QString const &
ConfigFile::fileName()
  const {
  return m_fileName;
}

void
ConfigFile::setFileName(QString const &name) {
  m_fileName = name;
}

ConfigFilePtr
ConfigFile::open(QString const &fileName) {
  auto configFile = openInternal(fileName);
  if (configFile)
    configFile->load();

  return configFile;
}

ConfigFile::Type
ConfigFile::determineType(QString const &fileName) {
  if (!QFileInfo{fileName}.exists())
    return UnknownType;

  QFile file{fileName};
  if (!file.open(QIODevice::ReadOnly))
    return UnknownType;

  auto firstChar = ' ';
  auto charRead  = file.getChar(&firstChar);

  file.close();

  return !charRead        ? UnknownType
       : firstChar == '[' ? Ini
       : firstChar == '{' ? Json
       :                    UnknownType;
}

ConfigFilePtr
ConfigFile::openInternal(QString const &fileName) {
  auto const type = determineType(fileName);

  return type == Ini  ? ConfigFilePtr{new IniConfigFile{fileName}}
       : type == Json ? ConfigFilePtr{new JsonConfigFile{fileName}}
       :                ConfigFilePtr{};
}

ConfigFilePtr
ConfigFile::create(QString const &fileName) {
  QFile{fileName}.remove();
  return ConfigFilePtr{new JsonConfigFile{fileName}};
}

}}}
