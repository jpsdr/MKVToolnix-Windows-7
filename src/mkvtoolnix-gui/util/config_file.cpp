#include "common/common_pch.h"

#include <QFile>
#include <QFileInfo>
#include <QSettings>

#include "mkvtoolnix-gui/util/config_file.h"
#include "mkvtoolnix-gui/util/ini_config_file.h"

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

ConfigFilePtr
ConfigFile::openInternal(QString const &fileName) {
  if (!QFileInfo{fileName}.exists())
    return ConfigFilePtr{new IniConfigFile{fileName}}; // TODO: replace by Json

  QFile file{fileName};
  if (!file.open(QIODevice::ReadOnly))
    return ConfigFilePtr{new IniConfigFile{fileName}}; // TODO: replace by Json

  auto firstChar = ' ';
  auto charRead  = file.getChar(&firstChar);

  file.close();

  if (charRead && (firstChar == '['))
    return ConfigFilePtr{new IniConfigFile{fileName}};

  return ConfigFilePtr{new IniConfigFile{fileName}}; // TODO: replace by Json
}

ConfigFilePtr
ConfigFile::create(QString const &fileName) {
  QFile{fileName}.remove();
  return ConfigFilePtr{new IniConfigFile{fileName}}; // TODO: replace by Json
}

}}}
