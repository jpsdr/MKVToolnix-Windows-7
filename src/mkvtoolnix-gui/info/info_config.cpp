#include "common/common_pch.h"

#include <QFile>

#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/info/info_config.h"

namespace mtx::gui::Info {

using namespace mtx::gui;

InfoConfig::InfoConfig(QString const &fileName)
  : m_configFileName{fileName}
{
}

InfoConfig::~InfoConfig() {
}

InfoConfig &
InfoConfig::operator =(InfoConfig const &other) {
  if (&other == this)
    return *this;

  m_sourceFileName      = other.m_sourceFileName;
  m_destinationFileName = other.m_destinationFileName;
  m_calcChecksums       = other.m_calcChecksums;
  m_showSummary         = other.m_showSummary;
  m_showHexdump         = other.m_showHexdump;
  m_showSize            = other.m_showSize;
  m_showTrackInfo       = other.m_showTrackInfo;
  m_hexPositions        = other.m_hexPositions;
  m_hexdumpMaxSize      = other.m_hexdumpMaxSize;
  m_verbose             = other.m_verbose;

  return *this;
}

void
InfoConfig::load(QString const &fileName) {
  auto fileNameToOpen = fileName.isEmpty() ? m_configFileName : fileName;

  if (fileNameToOpen.isEmpty())
    throw InvalidSettingsX{};

  auto settings = Util::ConfigFile::open(fileNameToOpen);
  if (!settings)
    throw InvalidSettingsX{};

  load(*settings);

  m_configFileName = fileNameToOpen;
}
void
InfoConfig::load(Util::ConfigFile &settings) {
  reset();

  // Check supported config file version
  if (settings.childGroups().contains(App::settingsBaseGroupName())) {
    settings.beginGroup(App::settingsBaseGroupName());
    if (   (settings.value("version", std::numeric_limits<unsigned int>::max()).toUInt() > Util::ConfigFile::MtxCfgVersion)
        || (settings.value("type").toString() != settingsType()))
      throw InvalidSettingsX{};
    settings.endGroup();
  }

  settings.beginGroup("info");

  m_configFileName      = settings.value("configFileName").toString();
  m_sourceFileName      = settings.value("sourceFileName").toString();
  m_destinationFileName = settings.value("destinationFileName").toString();
  m_calcChecksums       = settings.value("calcChecksums").toBool();
  m_showSummary         = settings.value("showSummary").toBool();
  m_showHexdump         = settings.value("showHexdump").toBool();
  m_showSize            = settings.value("showSize").toBool();
  m_showTrackInfo       = settings.value("showTrackInfo").toBool();
  m_hexPositions        = settings.value("hexPositions").toBool();
  m_hexdumpMaxSize      = settings.value("hexdumpMaxSize", 16).toInt();
  m_verbose             = settings.value("verbose").toInt();

  settings.endGroup();
}

void
InfoConfig::save(Util::ConfigFile &settings)
  const {
  settings.beginGroup(App::settingsBaseGroupName());
  settings.setValue("version", Util::ConfigFile::MtxCfgVersion);
  settings.setValue("type",    settingsType());
  settings.endGroup();

  settings.beginGroup("info");

  settings.setValue("configFileName",      m_configFileName);
  settings.setValue("sourceFileName",      m_sourceFileName);
  settings.setValue("destinationFileName", m_destinationFileName);
  settings.setValue("calcChecksums",       m_calcChecksums);
  settings.setValue("showSummary",         m_showSummary);
  settings.setValue("showHexdump",         m_showHexdump);
  settings.setValue("showSize",            m_showSize);
  settings.setValue("showTrackInfo",       m_showTrackInfo);
  settings.setValue("hexPositions",        m_hexPositions);
  settings.setValue("hexdumpMaxSize",      m_hexdumpMaxSize);
  settings.setValue("verbose",             m_verbose);

  settings.endGroup();
}

void
InfoConfig::save(QString const &fileName) {
  if (!fileName.isEmpty())
    m_configFileName = fileName;
  if (m_configFileName.isEmpty())
    return;

  QFile::remove(m_configFileName);
  auto settings = Util::ConfigFile::create(m_configFileName);
  save(*settings);
  settings->save();
}

void
InfoConfig::reset() {
  *this = InfoConfig{};
}

InfoConfigPtr
InfoConfig::loadSettings(QString const &fileName) {
  auto config = std::make_shared<InfoConfig>(fileName);
  config->load();

  return config;
}

QString
InfoConfig::settingsType() {
  return Q("InfoConfig");
}

}
