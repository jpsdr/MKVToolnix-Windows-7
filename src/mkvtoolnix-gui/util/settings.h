#pragma once

#include "common/common_pch.h"

#include <QDateTime>
#include <QDir>
#include <QString>
#include <QTabWidget>
#include <QVariant>

#include "common/translation.h"

class QSettings;
class QSplitter;

namespace mtx { namespace gui { namespace Util {

class Settings: public QObject {
  Q_OBJECT;
  Q_ENUMS(RunProgramForEvent);

public:
  enum RunProgramType {
    Min,
    ExecuteProgram,
    PlayAudioFile,
    ShutDownComputer,
    HibernateComputer,
    SleepComputer,
    Max,
    Default = ExecuteProgram,
  };

  enum RunProgramForEvent {
    RunNever                         = 0x00,
    RunAfterJobQueueFinishes         = 0x01,
    RunAfterJobCompletesSuccessfully = 0x02,
    RunAfterJobCompletesWithErrors   = 0x04,
  };

  Q_DECLARE_FLAGS(RunProgramForEvents, RunProgramForEvent);

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
    ToRelativeOfFirstInputFile,
  };

  enum class SetDefaultLanguagePolicy {
    OnlyIfAbsent = 0,
    IfAbsentOrUndetermined,
  };

  enum class JobRemovalPolicy {
    Never,
    IfSuccessful,
    IfWarningsFound,
    Always,
  };

  enum class ClearMergeSettingsAction {
    None,
    NewSettings,
    RemoveInputFiles,
    CloseSettings,
  };

  enum class MergeAddingAppendingFilesPolicy {
    Ask,
    Add,
    AddToNew,
    AddEachToNew,
    Append,
    AddAdditionalParts,
  };

  enum class HeaderEditorDroppedFilesPolicy {
    Ask,
    Open,
    AddAttachments,
  };

  enum class TrackPropertiesLayout {
    HorizontalScrollArea,
    HorizontalTwoColumns,
    VerticalTabWidget,
  };

  class RunProgramConfig {
  public:
    RunProgramType m_type{RunProgramType::ExecuteProgram};
    bool m_active{true};
    QString m_name;
    RunProgramForEvents m_forEvents{};
    QStringList m_commandLine;
    QString m_audioFile;
    unsigned int m_volume{75};

    bool isValid() const;
    QString validate() const;
    QString name() const;

  private:
    QString nameForExternalProgram() const;
    QString nameForPlayAudioFile() const;
  };

  using RunProgramConfigPtr  = std::shared_ptr<RunProgramConfig>;
  using RunProgramConfigList = QList<RunProgramConfigPtr>;

  QString m_defaultAudioTrackLanguage, m_defaultVideoTrackLanguage, m_defaultSubtitleTrackLanguage;
  SetDefaultLanguagePolicy m_whenToSetDefaultLanguage;
  QString m_chapterNameTemplate, m_defaultChapterLanguage, m_defaultChapterCountry, m_ceTextFileCharacterSet, m_defaultSubtitleCharset, m_defaultAdditionalMergeOptions;
  QStringList m_oftenUsedLanguages, m_oftenUsedCountries, m_oftenUsedCharacterSets;
  bool m_oftenUsedLanguagesOnly, m_oftenUsedCountriesOnly, m_oftenUsedCharacterSetsOnly;
  ProcessPriority m_priority;
  double m_probeRangePercentage;
  QTabWidget::TabPosition m_tabPosition;
  QDir m_lastOpenDir, m_lastOutputDir, m_lastConfigDir;
  bool m_setAudioDelayFromFileName, m_autoSetFileTitle, m_autoClearFileTitle, m_disableCompressionForAllTrackTypes, m_disableDefaultTrackForSubtitles, m_mergeAlwaysShowOutputFileControls, m_dropLastChapterFromBlurayPlaylist;
  ClearMergeSettingsAction m_clearMergeSettings;
  MergeAddingAppendingFilesPolicy m_mergeAddingAppendingFilesPolicy, m_mergeLastAddingAppendingDecision;
  HeaderEditorDroppedFilesPolicy m_headerEditorDroppedFilesPolicy;
  TrackPropertiesLayout m_mergeTrackPropertiesLayout;

  OutputFileNamePolicy m_outputFileNamePolicy;
  bool m_autoDestinationOnlyForVideoFiles;
  QDir m_relativeOutputDir, m_fixedOutputDir;
  bool m_uniqueOutputFileNames, m_autoClearOutputFileName;

  ScanForPlaylistsPolicy m_scanForPlaylistsPolicy;
  unsigned int m_minimumPlaylistDuration;

  JobRemovalPolicy m_jobRemovalPolicy;
  bool m_removeOldJobs;
  int m_removeOldJobsDays;
  bool m_useDefaultJobDescription, m_showOutputOfAllJobs, m_switchToJobOutputAfterStarting, m_resetJobWarningErrorCountersOnExit;

  bool m_checkForUpdates;
  QDateTime m_lastUpdateCheck;

  bool m_disableAnimations, m_showToolSelector, m_warnBeforeClosingModifiedTabs, m_warnBeforeAbortingJobs, m_warnBeforeOverwriting, m_showMoveUpDownButtons;
  QString m_uiLocale, m_uiFontFamily;
  int m_uiFontPointSize;

  bool m_enableMuxingTracksByLanguage, m_enableMuxingAllVideoTracks, m_enableMuxingAllAudioTracks, m_enableMuxingAllSubtitleTracks;
  QStringList m_enableMuxingTracksByTheseLanguages;

  QHash<QString, QList<int> > m_splitterSizes;

  QString m_mediaInfoExe;

  RunProgramConfigList m_runProgramConfigurations;

public:
  Settings();
  void load();
  void save() const;

  QString priorityAsString() const;
  QString actualMkvmergeExe() const;

  void setValue(QString const &group, QString const &key, QVariant const &value);
  QVariant value(QString const &group, QString const &key, QVariant const &defaultValue = QVariant{}) const;

  void handleSplitterSizes(QSplitter *splitter);
  void restoreSplitterSizes(QSplitter *splitter);

  QString localeToUse(QString const &requestedLocale = {}) const;

  QString lastOpenDirPath() const;
  QString lastConfigDirPath() const;

public slots:
  void storeSplitterSizes();

protected:
  void loadDefaults(QSettings &reg, QString const &guiVersion);
  void loadSplitterSizes(QSettings &reg);
  void loadRunProgramConfigurations(QSettings &reg);

  void saveDefaults(QSettings &reg) const;
  void saveSplitterSizes(QSettings &reg) const;
  void saveRunProgramConfigurations(QSettings &reg) const;

  void addDefaultRunProgramConfigurations(QSettings &reg);
  void addDefaultRunProgramConfigurationForType(QSettings &reg, RunProgramType type, std::function<void(RunProgramConfig &)> const &modifier = nullptr);
  bool fixDefaultAudioFileNameBug();

protected:
  static Settings s_settings;

  static void withGroup(QString const &group, std::function<void(QSettings &)> worker);

public:
  static Settings &get();
  static void change(std::function<void(Settings &)> worker);
  static std::unique_ptr<QSettings> registry();

  static void runOncePerVersion(QString const &topic, std::function<void()> worker);

  static QString exeWithPath(QString const &exe);

  static void migrateFromRegistry();
  static void convertOldSettings();

  static QString iniFileLocation();
  static QString iniFileName();

  static QString cacheDirLocation(QString const &subDir);
  static QString prepareCacheDir(QString const &subDir);
};

// extern Settings g_settings;

}}}

Q_DECLARE_OPERATORS_FOR_FLAGS(mtx::gui::Util::Settings::RunProgramForEvents);
