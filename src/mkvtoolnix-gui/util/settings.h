#ifndef MTX_MKVTOOLNIX_GUI_UTIL_SETTINGS_H
#define MTX_MKVTOOLNIX_GUI_UTIL_SETTINGS_H

#include "common/common_pch.h"

#include <QDateTime>
#include <QDir>
#include <QString>
#include <QVariant>

class QSettings;

namespace mtx { namespace gui { namespace Util {

class Settings: public QObject {
  Q_OBJECT;
public:
  enum ProcessPriority {
    LowestPriority = 0,
    LowPriority,
    NormalPriority,
    HighPriority,
    HighestPriority,
  };

  enum ScanForPlaylistsPolicy {
    AskBeforeScanning = 0,
    AlwaysScan,
    NeverScan,
  };

  enum OutputFileNamePolicy {
    DontSetOutputFileName = 0,
    ToPreviousDirectory,
    ToFixedDirectory,
    ToParentOfFirstInputFile,
    ToSameAsFirstInputFile,
  };

  enum class JobRemovalPolicy {
    Never,
    IfSuccessful,
    IfWarningsFound,
    Always,
  };

  QString m_defaultTrackLanguage, m_defaultChapterLanguage, m_defaultChapterCountry, m_defaultSubtitleCharset;
  QStringList m_oftenUsedLanguages, m_oftenUsedCountries, m_oftenUsedCharacterSets;
  ProcessPriority m_priority;
  QDir m_lastOpenDir, m_lastOutputDir, m_lastConfigDir, m_lastMatroskaFileDir;
  bool m_setAudioDelayFromFileName, m_autoSetFileTitle;

  OutputFileNamePolicy m_outputFileNamePolicy;
  QDir m_fixedOutputDir;
  bool m_uniqueOutputFileNames;

  ScanForPlaylistsPolicy m_scanForPlaylistsPolicy;
  unsigned int m_minimumPlaylistDuration;

  JobRemovalPolicy m_jobRemovalPolicy;

  bool m_checkForUpdates;
  QDateTime m_lastUpdateCheck;

public:
  Settings();
  void load();
  void save() const;

  QString getPriorityAsString() const;
  QString actualMkvmergeExe() const;

  void setValue(QString const &group, QString const &key, QVariant const &value);
  QVariant value(QString const &group, QString const &key, QVariant const &defaultValue = QVariant{}) const;

protected:
  static Settings s_settings;

  static void withGroup(QString const &group, std::function<void(QSettings &)> worker);

public:
  static Settings &get();
  static std::unique_ptr<QSettings> getRegistry();

  static QString exeWithPath(QString const &exe);
};

// extern Settings g_settings;

}}}

#endif  // MTX_MKVTOOLNIX_GUI_UTIL_SETTINGS_H
