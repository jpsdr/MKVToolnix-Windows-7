#pragma once

#include "common/common_pch.h"

#include <QString>

#include "mkvtoolnix-gui/util/config_file.h"

namespace mtx { namespace gui { namespace Info {

class InvalidSettingsX: public exception {
public:
  virtual const char *what() const throw() {
    return "invalid settings in file";
  }
};

class InfoConfig;
using InfoConfigPtr = std::shared_ptr<InfoConfig>;

class InfoConfig {
public:
  QString m_configFileName, m_sourceFileName, m_destinationFileName;
  bool m_calcChecksums{}, m_showSummary{}, m_showHexdump{}, m_showSize{}, m_showTrackInfo{}, m_hexPositions{};
  int m_hexdumpMaxSize{16}, m_verbose{};

public:
  InfoConfig(QString const &fileName = QString{""});
  InfoConfig(InfoConfig const &other) = default;
  virtual ~InfoConfig();
  InfoConfig &operator =(InfoConfig const &other);

  virtual void load(Util::ConfigFile &settings);
  virtual void load(QString const &fileName = QString{""});
  virtual void save(Util::ConfigFile &settings) const;
  virtual void save(QString const &fileName = QString{""});
  virtual void reset();

public:
  static InfoConfigPtr loadSettings(QString const &fileName);
  static QString settingsType();
};

}}}
