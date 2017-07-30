#include "common/common_pch.h"

#include <QDebug>
#include <QRegularExpression>
#include <QScreen>
#include <QSettings>
#include <QSplitter>
#include <QStandardPaths>

#include "common/extern_data.h"
#include "common/iso639.h"
#include "common/qt.h"
#include "common/version.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/jobs/program_runner.h"
#include "mkvtoolnix-gui/util/file_dialog.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/string.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx { namespace gui { namespace Util {

QString
Settings::RunProgramConfig::validate()
  const {
  return (m_type == RunProgramType::ExecuteProgram) && (m_commandLine.isEmpty() || m_commandLine.value(0).isEmpty()) ? QY("The program to execute hasn't been set yet.")
       : (m_type == RunProgramType::PlayAudioFile)  && m_audioFile.isEmpty()                                         ? QY("The audio file to play hasn't been set yet.")
       :                                                                                                               QString{};
}

bool
Settings::RunProgramConfig::isValid()
  const {
  return validate().isEmpty();
}

QString
Settings::RunProgramConfig::name()
  const {
  if (!m_name.isEmpty())
    return m_name;

  return m_type == RunProgramType::ExecuteProgram    ? nameForExternalProgram()
       : m_type == RunProgramType::PlayAudioFile     ? nameForPlayAudioFile()
       : m_type == RunProgramType::ShutDownComputer  ? QY("Shut down the computer")
       : m_type == RunProgramType::HibernateComputer ? QY("Hibernate the computer")
       : m_type == RunProgramType::SleepComputer     ? QY("Sleep the computer")
       :                                               Q("unknown");
}

QString
Settings::RunProgramConfig::nameForExternalProgram()
  const {
  if (m_commandLine.isEmpty())
    return QY("Execute a program");

  auto program = m_commandLine.value(0);
  program.replace(QRegularExpression{Q(".*[/\\\\]")}, Q(""));

  return QY("Execute program '%1'").arg(program);
}

QString
Settings::RunProgramConfig::nameForPlayAudioFile()
  const {
  if (m_audioFile.isEmpty())
    return QY("Play an audio file");

  auto audioFile = m_audioFile;
  audioFile.replace(QRegularExpression{Q(".*[/\\\\]")}, Q(""));

  return QY("Play audio file '%1'").arg(audioFile);
}

Settings Settings::s_settings;

Settings::Settings() {
}

void
Settings::change(std::function<void(Settings &)> worker) {
  worker(s_settings);
  s_settings.save();
}

Settings &
Settings::get() {
  return s_settings;
}

QString
Settings::iniFileLocation() {
#if defined(SYS_WINDOWS)
  if (!App::isInstalled())
    return App::applicationDirPath();

  return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
#else
  return Q("%1/%2/%3").arg(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)).arg(App::organizationName()).arg(App::applicationName());
#endif
}

QString
Settings::cacheDirLocation(QString const &subDir) {
  QString dir;

#if defined(SYS_WINDOWS)
  if (!App::isInstalled())
    dir = Q("%1/cache").arg(App::applicationDirPath());
#endif

  if (dir.isEmpty())
    dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);

  if (!subDir.isEmpty())
    dir = Q("%1/%2").arg(dir).arg(subDir);

  return QDir::toNativeSeparators(dir);
}

QString
Settings::prepareCacheDir(QString const &subDir) {
  auto dirName = cacheDirLocation(subDir);
  QDir{dirName}.mkpath(dirName);

  return QDir::toNativeSeparators(dirName);
}

QString
Settings::iniFileName() {
  return Q("%1/mkvtoolnix-gui.ini").arg(iniFileLocation());
}

void
Settings::migrateFromRegistry() {
#if defined(SYS_WINDOWS)
  // Migration of settings from the Windows registry into an .ini file
  // due to performance issues.

  // If this is the portable version the no settings have to be migrated.
  if (!App::isInstalled())
    return;

  // Don't do anything if such a file exists already.
  auto targetFileName = iniFileName();
  if (QFileInfo{targetFileName}.exists())
    return;

  // Copy all settings.
  QSettings target{targetFileName, QSettings::IniFormat};
  QSettings source{};

  for (auto const &key : source.allKeys())
    target.setValue(key, source.value(key));

  // Ensure the new file is written and remove the keys from the
  // registry.
  target.sync();
  source.clear();
  source.sync();

#else
  // Rename file from .conf to .ini in order to be consistent.
  auto target = QFileInfo{ iniFileName() };
  auto source = QFileInfo{ Q("%1/%2.conf").arg(target.absolutePath()).arg(target.baseName()) };

  if (source.exists())
    QFile{ source.absoluteFilePath() }.rename(target.filePath());
#endif
}

std::unique_ptr<QSettings>
Settings::registry() {
  return std::make_unique<QSettings>(iniFileName(), QSettings::IniFormat);
}

void
Settings::convertOldSettings() {
  auto reg = registry();

  // mergeAlwaysAddDroppedFiles → mergeAddingAppendingFilesPolicy
  reg->beginGroup("settings");
  auto mergeAlwaysAddDroppedFiles = reg->value("mergeAlwaysAddDroppedFiles");

  if (mergeAlwaysAddDroppedFiles.isValid())
    reg->setValue("mergeAddingAppendingFilesPolicy", static_cast<int>(MergeAddingAppendingFilesPolicy::Add));

  reg->remove("mergeAlwaysAddDroppedFiles");
  reg->endGroup();

  // defaultTrackLanguage → defaultAudioTrackLanguage, defaultVideoTrackLanguage, defaultSubtitleTrackLanguage;
  reg->beginGroup("settings");

  auto defaultTrackLanguage = reg->value("defaultTrackLanguage");
  reg->remove("defaultTrackLanguage");

  if (defaultTrackLanguage.isValid()) {
    reg->setValue("defaultAudioTrackLanguage",    defaultTrackLanguage.toString());
    reg->setValue("defaultVideoTrackLanguage",    defaultTrackLanguage.toString());
    reg->setValue("defaultSubtitleTrackLanguage", defaultTrackLanguage.toString());
  }

  // mergeUseVerticalInputLayout → mergeTrackPropertiesLayout
  auto mergeUseVerticalInputLayout = reg->value("mergeUseVerticalInputLayout");
  reg->remove("mergeUseVerticalInputLayout");

  if (mergeUseVerticalInputLayout.isValid())
    reg->setValue("mergeTrackPropertiesLayout", static_cast<int>(mergeUseVerticalInputLayout.toBool() ? TrackPropertiesLayout::VerticalTabWidget : TrackPropertiesLayout::HorizontalScrollArea));

  reg->endGroup();
}

void
Settings::load() {
  convertOldSettings();

  auto regPtr      = registry();
  auto &reg        = *regPtr;
  auto defaultFont = defaultUiFont();

  reg.beginGroup("info");
  auto guiVersion                      = reg.value("guiVersion").toString();
  reg.endGroup();

  reg.beginGroup("settings");
  m_priority                           = static_cast<ProcessPriority>(reg.value("priority", static_cast<int>(NormalPriority)).toInt());
  m_probeRangePercentage               = reg.value("probeRangePercentage", 0.3).toDouble();
  m_tabPosition                        = static_cast<QTabWidget::TabPosition>(reg.value("tabPosition", static_cast<int>(QTabWidget::North)).toInt());
  m_lastOpenDir                        = QDir{reg.value("lastOpenDir").toString()};
  m_lastOutputDir                      = QDir{reg.value("lastOutputDir").toString()};
  m_lastConfigDir                      = QDir{reg.value("lastConfigDir").toString()};

  m_oftenUsedLanguages                 = reg.value("oftenUsedLanguages").toStringList();
  m_oftenUsedCountries                 = reg.value("oftenUsedCountries").toStringList();
  m_oftenUsedCharacterSets             = reg.value("oftenUsedCharacterSets").toStringList();

  m_oftenUsedLanguagesOnly             = reg.value("oftenUsedLanguagesOnly",     false).toBool();;
  m_oftenUsedCountriesOnly             = reg.value("oftenUsedCountriesOnly",     false).toBool();;
  m_oftenUsedCharacterSetsOnly         = reg.value("oftenUsedCharacterSetsOnly", false).toBool();;

  m_scanForPlaylistsPolicy             = static_cast<ScanForPlaylistsPolicy>(reg.value("scanForPlaylistsPolicy", static_cast<int>(AskBeforeScanning)).toInt());
  m_minimumPlaylistDuration            = reg.value("minimumPlaylistDuration", 120).toUInt();

  m_setAudioDelayFromFileName          = reg.value("setAudioDelayFromFileName", true).toBool();
  m_autoSetFileTitle                   = reg.value("autoSetFileTitle",          true).toBool();
  m_autoClearFileTitle                 = reg.value("autoClearFileTitle",        m_autoSetFileTitle).toBool();
  m_clearMergeSettings                 = static_cast<ClearMergeSettingsAction>(reg.value("clearMergeSettings", static_cast<int>(ClearMergeSettingsAction::None)).toInt());
  m_disableCompressionForAllTrackTypes = reg.value("disableCompressionForAllTrackTypes", false).toBool();
  m_disableDefaultTrackForSubtitles    = reg.value("disableDefaultTrackForSubtitles",    false).toBool();
  m_mergeAlwaysShowOutputFileControls  = reg.value("mergeAlwaysShowOutputFileControls",  true).toBool();
  m_mergeTrackPropertiesLayout         = static_cast<TrackPropertiesLayout>(reg.value("mergeTrackPropertiesLayout", static_cast<int>(TrackPropertiesLayout::HorizontalScrollArea)).toInt());
  m_mergeAddingAppendingFilesPolicy    = static_cast<MergeAddingAppendingFilesPolicy>(reg.value("mergeAddingAppendingFilesPolicy", static_cast<int>(MergeAddingAppendingFilesPolicy::Ask)).toInt());
  m_mergeLastAddingAppendingDecision   = static_cast<MergeAddingAppendingFilesPolicy>(reg.value("mergeLastAddingAppendingDecision", static_cast<int>(MergeAddingAppendingFilesPolicy::Add)).toInt());
  m_headerEditorDroppedFilesPolicy     = static_cast<HeaderEditorDroppedFilesPolicy>(reg.value("headerEditorDroppedFilesPolicy", static_cast<int>(HeaderEditorDroppedFilesPolicy::Ask)).toInt());

  m_outputFileNamePolicy               = static_cast<OutputFileNamePolicy>(reg.value("outputFileNamePolicy", static_cast<int>(ToSameAsFirstInputFile)).toInt());
  m_autoDestinationOnlyForVideoFiles   = reg.value("autoDestinationOnlyForVideoFiles", false).toBool();
  m_relativeOutputDir                  = QDir{reg.value("relativeOutputDir").toString()};
  m_fixedOutputDir                     = QDir{reg.value("fixedOutputDir").toString()};
  m_uniqueOutputFileNames              = reg.value("uniqueOutputFileNames",   true).toBool();
  m_autoClearOutputFileName            = reg.value("autoClearOutputFileName", m_outputFileNamePolicy != DontSetOutputFileName).toBool();

  m_enableMuxingTracksByLanguage       = reg.value("enableMuxingTracksByLanguage", false).toBool();
  m_enableMuxingAllVideoTracks         = reg.value("enableMuxingAllVideoTracks", true).toBool();
  m_enableMuxingAllAudioTracks         = reg.value("enableMuxingAllAudioTracks", false).toBool();
  m_enableMuxingAllSubtitleTracks      = reg.value("enableMuxingAllSubtitleTracks", false).toBool();
  m_enableMuxingTracksByTheseLanguages = reg.value("enableMuxingTracksByTheseLanguages").toStringList();

  m_useDefaultJobDescription           = reg.value("useDefaultJobDescription",       false).toBool();
  m_showOutputOfAllJobs                = reg.value("showOutputOfAllJobs",            true).toBool();
  m_switchToJobOutputAfterStarting     = reg.value("switchToJobOutputAfterStarting", false).toBool();
  m_resetJobWarningErrorCountersOnExit = reg.value("resetJobWarningErrorCountersOnExit", false).toBool();
  m_jobRemovalPolicy                   = static_cast<JobRemovalPolicy>(reg.value("jobRemovalPolicy", static_cast<int>(JobRemovalPolicy::Never)).toInt());
  m_removeOldJobs                      = reg.value("removeOldJobs",                                  true).toBool();
  m_removeOldJobsDays                  = reg.value("removeOldJobsDays",                              14).toInt();

  m_disableAnimations                  = reg.value("disableAnimations", false).toBool();
  m_showToolSelector                   = reg.value("showToolSelector", true).toBool();
  m_warnBeforeClosingModifiedTabs      = reg.value("warnBeforeClosingModifiedTabs", true).toBool();
  m_warnBeforeAbortingJobs             = reg.value("warnBeforeAbortingJobs", true).toBool();
  m_warnBeforeOverwriting              = reg.value("warnBeforeOverwriting",  true).toBool();
  m_showMoveUpDownButtons              = reg.value("showMoveUpDownButtons", false).toBool();

  m_chapterNameTemplate                = reg.value("chapterNameTemplate", QY("Chapter <NUM:2>")).toString();
  m_dropLastChapterFromBlurayPlaylist  = reg.value("dropLastChapterFromBlurayPlaylist", true).toBool();
  m_ceTextFileCharacterSet             = reg.value("ceTextFileCharacterSet").toString();

  m_mediaInfoExe                       = reg.value("mediaInfoExe", Q("mediainfo-gui")).toString();

#if defined(HAVE_LIBINTL_H)
  m_uiLocale                           = reg.value("uiLocale").toString();
#endif
  m_uiFontFamily                       = reg.value("uiFontFamily",    defaultFont.family()).toString();
  m_uiFontPointSize                    = reg.value("uiFontPointSize", defaultFont.pointSize()).toInt();

  reg.beginGroup("updates");
  m_checkForUpdates                    = reg.value("checkForUpdates", true).toBool();
  m_lastUpdateCheck                    = reg.value("lastUpdateCheck", QDateTime{}).toDateTime();
  reg.endGroup();               // settings.updates
  reg.endGroup();               // settings

  loadDefaults(reg, guiVersion);
  loadSplitterSizes(reg);
  loadRunProgramConfigurations(reg);
  addDefaultRunProgramConfigurations(reg);
}

void
Settings::loadDefaults(QSettings &reg,
                       QString const &guiVersion) {
  reg.beginGroup("defaults");
  m_defaultAudioTrackLanguage          = reg.value("defaultAudioTrackLanguage",    Q("und")).toString();
  m_defaultVideoTrackLanguage          = reg.value("defaultVideoTrackLanguage",    Q("und")).toString();
  m_defaultSubtitleTrackLanguage       = reg.value("defaultSubtitleTrackLanguage", Q("und")).toString();
  m_whenToSetDefaultLanguage           = static_cast<SetDefaultLanguagePolicy>(reg.value("whenToSetDefaultLanguage",     static_cast<int>(SetDefaultLanguagePolicy::IfAbsentOrUndetermined)).toInt());
  m_defaultChapterLanguage             = reg.value("defaultChapterLanguage", Q("und")).toString();
  m_defaultChapterCountry              = Util::mapToTopLevelCountryCode(reg.value("defaultChapterCountry").toString());
  auto subtitleCharset                 = reg.value("defaultSubtitleCharset").toString();
  m_defaultSubtitleCharset             = guiVersion.isEmpty() && (subtitleCharset == Q("ISO-8859-15")) ? Q("") : subtitleCharset; // Fix for a bug in versions prior to 8.2.0.
  m_defaultAdditionalMergeOptions      = reg.value("defaultAdditionalMergeOptions").toString();
  reg.endGroup();               // defaults
}

void
Settings::loadSplitterSizes(QSettings &reg) {
  reg.beginGroup("splitterSizes");

  m_splitterSizes.clear();
  for (auto const &name : reg.childKeys()) {
    auto sizes = reg.value(name).toList();
    for (auto const &size : sizes)
      m_splitterSizes[name] << size.toInt();
  }

  reg.endGroup();               // splitterSizes

  if (m_oftenUsedLanguages.isEmpty())
    for (auto const &languageCode : g_popular_language_codes)
      m_oftenUsedLanguages << Q(languageCode);

  if (m_oftenUsedCountries.isEmpty())
    for (auto const &countryCode : g_popular_country_codes)
      m_oftenUsedCountries << Util::mapToTopLevelCountryCode(Q(countryCode));

  if (m_oftenUsedCharacterSets.isEmpty())
    for (auto const &characterSet : g_popular_character_sets)
      m_oftenUsedCharacterSets << Q(characterSet);

  if (ToParentOfFirstInputFile == m_outputFileNamePolicy) {
    m_outputFileNamePolicy = ToRelativeOfFirstInputFile;
    m_relativeOutputDir    = Q("..");
  }
}

void
Settings::loadRunProgramConfigurations(QSettings &reg) {
  m_runProgramConfigurations.clear();

  reg.beginGroup("runProgramConfigurations");

  auto groups = reg.childGroups();
  groups.sort();

  for (auto const &group : groups) {
    auto cfg = std::make_shared<RunProgramConfig>();

    reg.beginGroup(group);
    cfg->m_active      = reg.value("active", true).toBool();
    cfg->m_name        = reg.value("name").toString();
    auto type          = reg.value("type", static_cast<int>(RunProgramType::ExecuteProgram)).toInt();
    cfg->m_type        = (type > static_cast<int>(RunProgramType::Min)) && (type < static_cast<int>(RunProgramType::Max)) ? static_cast<RunProgramType>(type) : RunProgramType::Default;
    cfg->m_forEvents   = static_cast<RunProgramForEvents>(reg.value("forEvents").toInt());
    cfg->m_commandLine = reg.value("commandLine").toStringList();
    cfg->m_audioFile   = reg.value("audioFile").toString();
    cfg->m_volume      = std::min(reg.value("volume", 50).toUInt(), 100u);
    reg.endGroup();

    if (!cfg->m_active || cfg->isValid())
      m_runProgramConfigurations << cfg;
  }

  reg.endGroup();               // runProgramConfigurations
}

void
Settings::addDefaultRunProgramConfigurationForType(QSettings &reg,
                                                   RunProgramType type,
                                                   std::function<void(RunProgramConfig &)> const &modifier) {
  auto guard = Q("addedDefaultConfigurationType%1").arg(static_cast<int>(type));

  if (reg.value(guard).toBool() || !App::programRunner().isRunProgramTypeSupported(type))
    return;

  auto cfg      = std::make_shared<RunProgramConfig>();

  cfg->m_active = true;
  cfg->m_type   = type;

  if (modifier)
    modifier(*cfg);

  if (cfg->isValid()) {
    m_runProgramConfigurations << cfg;
    reg.setValue(guard, true);
  }
}

void
Settings::addDefaultRunProgramConfigurations(QSettings &reg) {
  reg.beginGroup("runProgramConfigurations");

  auto numConfigurationsBefore = m_runProgramConfigurations.count();

  addDefaultRunProgramConfigurationForType(reg, RunProgramType::PlayAudioFile, [](RunProgramConfig &cfg) { cfg.m_audioFile = App::programRunner().defaultAudioFileName(); });
  addDefaultRunProgramConfigurationForType(reg, RunProgramType::SleepComputer);
  addDefaultRunProgramConfigurationForType(reg, RunProgramType::HibernateComputer);
  addDefaultRunProgramConfigurationForType(reg, RunProgramType::ShutDownComputer);

  auto changed = fixDefaultAudioFileNameBug();

  if ((numConfigurationsBefore != m_runProgramConfigurations.count()) || changed)
    saveRunProgramConfigurations(reg);

  reg.endGroup();               // runProgramConfigurations
}

bool
Settings::fixDefaultAudioFileNameBug() {
#if defined(SYS_WINDOWS)
  // In version v11.0.0 the default audio file name is wrong:
  // <MTX_INSTALLATION_DIRECTORY>\sounds\… instead of
  // <MTX_INSTALLATION_DIRECTORY>\data\sounds\… where the default
  // sound files are actually installed. As each configuration is only
  // added once, update an existing configuration to the actual path.
  QRegularExpression wrongFileNameRE{"<MTX_INSTALLATION_DIRECTORY>[/\\\\]sounds[/\\\\]finished-1\\.ogg"};
  auto changed = false;

  for (auto const &config : m_runProgramConfigurations) {
    if (   (config->m_type != RunProgramType::PlayAudioFile)
        || !config->m_audioFile.contains(wrongFileNameRE))
      continue;

    config->m_audioFile = App::programRunner().defaultAudioFileName();
    changed             = true;
  }

  return changed;

#else
  return false;
#endif
}

QString
Settings::actualMkvmergeExe()
  const {
  return exeWithPath(Q("mkvmerge"));
}

void
Settings::save()
  const {
  auto regPtr = registry();
  auto &reg   = *regPtr;

  reg.beginGroup("info");
  reg.setValue("guiVersion",                         Q(get_current_version().to_string()));
  reg.endGroup();

  reg.beginGroup("settings");
  reg.setValue("priority",                           static_cast<int>(m_priority));
  reg.setValue("probeRangePercentage",               m_probeRangePercentage);
  reg.setValue("tabPosition",                        static_cast<int>(m_tabPosition));
  reg.setValue("lastOpenDir",                        m_lastOpenDir.path());
  reg.setValue("lastOutputDir",                      m_lastOutputDir.path());
  reg.setValue("lastConfigDir",                      m_lastConfigDir.path());

  reg.setValue("oftenUsedLanguages",                 m_oftenUsedLanguages);
  reg.setValue("oftenUsedCountries",                 m_oftenUsedCountries);
  reg.setValue("oftenUsedCharacterSets",             m_oftenUsedCharacterSets);

  reg.setValue("oftenUsedLanguagesOnly",             m_oftenUsedLanguagesOnly);
  reg.setValue("oftenUsedCountriesOnly",             m_oftenUsedCountriesOnly);
  reg.setValue("oftenUsedCharacterSetsOnly",         m_oftenUsedCharacterSetsOnly);

  reg.setValue("scanForPlaylistsPolicy",             static_cast<int>(m_scanForPlaylistsPolicy));
  reg.setValue("minimumPlaylistDuration",            m_minimumPlaylistDuration);

  reg.setValue("setAudioDelayFromFileName",          m_setAudioDelayFromFileName);
  reg.setValue("autoSetFileTitle",                   m_autoSetFileTitle);
  reg.setValue("autoClearFileTitle",                 m_autoClearFileTitle);
  reg.setValue("clearMergeSettings",                 static_cast<int>(m_clearMergeSettings));
  reg.setValue("disableCompressionForAllTrackTypes", m_disableCompressionForAllTrackTypes);
  reg.setValue("disableDefaultTrackForSubtitles",    m_disableDefaultTrackForSubtitles);
  reg.setValue("mergeAlwaysShowOutputFileControls",  m_mergeAlwaysShowOutputFileControls);
  reg.setValue("mergeTrackPropertiesLayout",         static_cast<int>(m_mergeTrackPropertiesLayout));
  reg.setValue("mergeAddingAppendingFilesPolicy",    static_cast<int>(m_mergeAddingAppendingFilesPolicy));
  reg.setValue("mergeLastAddingAppendingDecision",   static_cast<int>(m_mergeLastAddingAppendingDecision));
  reg.setValue("headerEditorDroppedFilesPolicy",     static_cast<int>(m_headerEditorDroppedFilesPolicy));

  reg.setValue("outputFileNamePolicy",               static_cast<int>(m_outputFileNamePolicy));
  reg.setValue("autoDestinationOnlyForVideoFiles",   m_autoDestinationOnlyForVideoFiles);
  reg.setValue("relativeOutputDir",                  m_relativeOutputDir.path());
  reg.setValue("fixedOutputDir",                     m_fixedOutputDir.path());
  reg.setValue("uniqueOutputFileNames",              m_uniqueOutputFileNames);
  reg.setValue("autoClearOutputFileName",            m_autoClearOutputFileName);

  reg.setValue("enableMuxingTracksByLanguage",       m_enableMuxingTracksByLanguage);
  reg.setValue("enableMuxingAllVideoTracks",         m_enableMuxingAllVideoTracks);
  reg.setValue("enableMuxingAllAudioTracks",         m_enableMuxingAllAudioTracks);
  reg.setValue("enableMuxingAllSubtitleTracks",      m_enableMuxingAllSubtitleTracks);
  reg.setValue("enableMuxingTracksByTheseLanguages", m_enableMuxingTracksByTheseLanguages);

  reg.setValue("useDefaultJobDescription",           m_useDefaultJobDescription);
  reg.setValue("showOutputOfAllJobs",                m_showOutputOfAllJobs);
  reg.setValue("switchToJobOutputAfterStarting",     m_switchToJobOutputAfterStarting);
  reg.setValue("resetJobWarningErrorCountersOnExit", m_resetJobWarningErrorCountersOnExit);
  reg.setValue("jobRemovalPolicy",                   static_cast<int>(m_jobRemovalPolicy));
  reg.setValue("removeOldJobs",                      m_removeOldJobs);
  reg.setValue("removeOldJobsDays",                  m_removeOldJobsDays);

  reg.setValue("disableAnimations",                  m_disableAnimations);
  reg.setValue("showToolSelector",                   m_showToolSelector);
  reg.setValue("warnBeforeClosingModifiedTabs",      m_warnBeforeClosingModifiedTabs);
  reg.setValue("warnBeforeAbortingJobs",             m_warnBeforeAbortingJobs);
  reg.setValue("warnBeforeOverwriting",              m_warnBeforeOverwriting);
  reg.setValue("showMoveUpDownButtons",              m_showMoveUpDownButtons);

  reg.setValue("chapterNameTemplate",                m_chapterNameTemplate);
  reg.setValue("dropLastChapterFromBlurayPlaylist",  m_dropLastChapterFromBlurayPlaylist);
  reg.setValue("ceTextFileCharacterSet",             m_ceTextFileCharacterSet);

  reg.setValue("uiLocale",                           m_uiLocale);
  reg.setValue("uiFontFamily",                       m_uiFontFamily);
  reg.setValue("uiFontPointSize",                    m_uiFontPointSize);

  reg.setValue("mediaInfoExe",                       m_mediaInfoExe);

  reg.beginGroup("updates");
  reg.setValue("checkForUpdates",                    m_checkForUpdates);
  reg.setValue("lastUpdateCheck",                    m_lastUpdateCheck);
  reg.endGroup();               // settings.updates
  reg.endGroup();               // settings

  saveDefaults(reg);
  saveSplitterSizes(reg);
  saveRunProgramConfigurations(reg);
}

void
Settings::saveDefaults(QSettings &reg)
  const {
  reg.beginGroup("defaults");
  reg.setValue("defaultAudioTrackLanguage",          m_defaultAudioTrackLanguage);
  reg.setValue("defaultVideoTrackLanguage",          m_defaultVideoTrackLanguage);
  reg.setValue("defaultSubtitleTrackLanguage",       m_defaultSubtitleTrackLanguage);
  reg.setValue("whenToSetDefaultLanguage",           static_cast<int>(m_whenToSetDefaultLanguage));
  reg.setValue("defaultChapterLanguage",             m_defaultChapterLanguage);
  reg.setValue("defaultChapterCountry",              m_defaultChapterCountry);
  reg.setValue("defaultSubtitleCharset",             m_defaultSubtitleCharset);
  reg.setValue("defaultAdditionalMergeOptions",      m_defaultAdditionalMergeOptions);
  reg.endGroup();               // defaults
}

void
Settings::saveSplitterSizes(QSettings &reg)
  const {
  reg.beginGroup("splitterSizes");
  for (auto const &name : m_splitterSizes.keys()) {
    auto sizes = QVariantList{};
    for (auto const &size : m_splitterSizes[name])
      sizes << size;
    reg.setValue(name, sizes);
  }
  reg.endGroup();               // splitterSizes

  saveRunProgramConfigurations(reg);
}

void
Settings::saveRunProgramConfigurations(QSettings &reg)
  const {
  reg.beginGroup("runProgramConfigurations");

  auto groups = reg.childGroups();
  groups.sort();

  for (auto const &group : groups)
    reg.remove(group);

  auto idx = 0;
  for (auto const &cfg : m_runProgramConfigurations) {
    reg.beginGroup(Q("%1").arg(++idx, 4, 10, Q('0')));
    reg.setValue("active",      cfg->m_active);
    reg.setValue("name",        cfg->m_name);
    reg.setValue("type",        static_cast<int>(cfg->m_type));
    reg.setValue("forEvents",   static_cast<int>(cfg->m_forEvents));
    reg.setValue("commandLine", cfg->m_commandLine);
    reg.setValue("audioFile",   cfg->m_audioFile);
    reg.setValue("volume",      cfg->m_volume);
    reg.endGroup();
  }

  reg.endGroup();               // runProgramConfigurations
}

QString
Settings::priorityAsString()
  const {
  return LowestPriority == m_priority ? Q("lowest")
       : LowPriority    == m_priority ? Q("lower")
       : NormalPriority == m_priority ? Q("normal")
       : HighPriority   == m_priority ? Q("higher")
       :                                Q("highest");
}

QString
Settings::exeWithPath(QString const &exe) {
  auto path          = bfs::path{ to_utf8(exe) };
  auto program       = path.filename();
  auto installPath   = bfs::path{ to_utf8(App::applicationDirPath()) };
  auto potentialExes = QList<bfs::path>{} << path << (installPath / path) << (installPath / ".." / path);

#if defined(SYS_WINDOWS)
  for (auto &potentialExe : potentialExes)
    potentialExe.replace_extension(bfs::path{"exe"});

  program.replace_extension(bfs::path{"exe"});
#endif  // SYS_WINDOWS

  for (auto const &potentialExe : potentialExes)
    if (bfs::exists(potentialExe))
      return to_qs(potentialExe.string());

  auto location = QStandardPaths::findExecutable(to_qs(program.string()));
  if (!location.isEmpty())
    return location;

  return exe;
}

void
Settings::setValue(QString const &group,
                   QString const &key,
                   QVariant const &value) {
  withGroup(group, [&key, &value](QSettings &reg) {
    reg.setValue(key, value);
  });
}

QVariant
Settings::value(QString const &group,
                QString const &key,
                QVariant const &defaultValue)
  const {
  auto result = QVariant{};

  withGroup(group, [&key, &defaultValue, &result](QSettings &reg) {
    result = reg.value(key, defaultValue);
  });

  return result;
}

void
Settings::withGroup(QString const &group,
                    std::function<void(QSettings &)> worker) {
  auto reg    = registry();
  auto groups = group.split(Q("/"));

  for (auto const &subGroup : groups)
    reg->beginGroup(subGroup);

  worker(*reg);

  for (auto idx = groups.size(); idx > 0; --idx)
    reg->endGroup();
}

void
Settings::handleSplitterSizes(QSplitter *splitter) {
  restoreSplitterSizes(splitter);
  connect(splitter, &QSplitter::splitterMoved, this, &Settings::storeSplitterSizes);
}

void
Settings::restoreSplitterSizes(QSplitter *splitter) {
 auto name   = splitter->objectName();
 auto &sizes = m_splitterSizes[name];

  if (sizes.isEmpty())
    for (auto idx = 0, numWidgets = splitter->count(); idx < numWidgets; ++idx)
      sizes << 1;

  splitter->setSizes(sizes);
}

void
Settings::storeSplitterSizes() {
  auto splitter = dynamic_cast<QSplitter *>(sender());
  if (splitter)
    m_splitterSizes[ splitter->objectName() ] = splitter->sizes();
  else
    qDebug() << "storeSplitterSize() signal from non-splitter" << sender() << sender()->objectName();
}

QString
Settings::localeToUse(QString const &requestedLocale)
  const {
  auto locale = to_utf8(requestedLocale);

#if defined(HAVE_LIBINTL_H)
  translation_c::initialize_available_translations();

  if (locale.empty())
    locale = to_utf8(m_uiLocale);

  if (-1 == translation_c::look_up_translation(locale))
    locale = "";

  if (locale.empty()) {
    locale = boost::regex_replace(translation_c::get_default_ui_locale(), boost::regex{"\\..*", boost::regex::perl}, "");
    if (-1 == translation_c::look_up_translation(locale))
      locale = "";
  }
#endif

  return to_qs(locale);
}

QString
Settings::lastConfigDirPath()
  const {
  return Util::dirPath(m_lastConfigDir);
}

QString
Settings::lastOpenDirPath()
  const {
  return Util::dirPath(m_lastOpenDir);
}

void
Settings::runOncePerVersion(QString const &topic,
                            std::function<void()> worker) {
  auto reg = registry();
  auto key = Q("runOncePerVersion/%1").arg(topic);

  auto lastRunInVersion       = reg->value(key).toString();
  auto lastRunInVersionNumber = version_number_t{to_utf8(lastRunInVersion)};
  auto currentVersionNumber   = get_current_version();

  if (   lastRunInVersionNumber.valid
      && !(lastRunInVersionNumber < currentVersionNumber))
    return;

  reg->setValue(key, Q(currentVersionNumber.to_string()));

  worker();
}

}}}
