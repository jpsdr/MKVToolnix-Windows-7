#include "common/common_pch.h"

#include <QColor>
#include <QDebug>
#include <QRegularExpression>
#include <QScreen>
#include <QSettings>
#include <QSplitter>
#include <QStandardPaths>

#include "common/chapters/chapters.h"
#include "common/character_sets.h"
#include "common/fs_sys_helpers.h"
#include "common/iso639.h"
#include "common/path.h"
#include "common/qt.h"
#include "common/random.h"
#include "common/sorting.h"
#include "common/version.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/jobs/program_runner.h"
#include "mkvtoolnix-gui/merge/command_line_dialog.h"
#include "mkvtoolnix-gui/merge/enums.h"
#include "mkvtoolnix-gui/util/file_dialog.h"
#include "mkvtoolnix-gui/util/font.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/settings_names.h"
#include "mkvtoolnix-gui/util/string.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx::gui::Util {

namespace {
void
convert8_1_0DefaultSubtitleCharset(version_number_t const &writtenByVersion) {
  auto reg = Settings::registry();

  reg->beginGroup(s_grpDefaults);
  if (   (writtenByVersion == version_number_t{"8.1.0"})
      && (reg->value(s_valDefaultSubtitleCharset).toString() == Q("ISO-8859-15"))) {
    // Fix for a bug in versions prior to 8.2.0.
    reg->remove(s_valDefaultSubtitleCharset);
  }
  reg->endGroup();
}


void
convert8_1_0DefaultTrackLanguage() {
  auto reg = Settings::registry();

  // defaultTrackLanguage → defaultAudioTrackLanguage, defaultVideoTrackLanguage, defaultSubtitleTrackLanguage;
  reg->beginGroup(s_grpSettings);

  auto defaultTrackLanguage = reg->value(s_valDefaultTrackLanguage);
  reg->remove(s_valDefaultTrackLanguage);

  if (defaultTrackLanguage.isValid()) {
    reg->setValue(s_valDefaultAudioTrackLanguage,    defaultTrackLanguage.toString());
    reg->setValue(s_valDefaultVideoTrackLanguage,    defaultTrackLanguage.toString());
    reg->setValue(s_valDefaultSubtitleTrackLanguage, defaultTrackLanguage.toString());
  }

  // mergeUseVerticalInputLayout → mergeTrackPropertiesLayout
  auto mergeUseVerticalInputLayout = reg->value(s_valMergeUseVerticalInputLayout);
  reg->remove(s_valMergeUseVerticalInputLayout);

  if (mergeUseVerticalInputLayout.isValid())
    reg->setValue(s_valMergeTrackPropertiesLayout, static_cast<int>(mergeUseVerticalInputLayout.toBool() ? Settings::TrackPropertiesLayout::VerticalTabWidget : Settings::TrackPropertiesLayout::HorizontalScrollArea));

  reg->endGroup();
}

void
convert37_0_0OftenUsedLanguages(version_number_t const &writtenByVersion) {
  // Update list of often-used language codes when updating from v37
  // or earlier due to the change that only often used languages are
  // shown by default. Unfortunately that list was rather short in
  // earlier releases, leading to confused users missing their
  // language.
  if (writtenByVersion > version_number_t{"37"})
    return;

  auto reg = Settings::registry();

  reg->beginGroup(s_grpSettings);
  reg->remove(s_valOftenUsedLanguages);
  reg->endGroup();
}

void
convert40_0_0PredefinedTrackNames() {
  auto reg = Settings::registry();

  // Predefined track names have been split up into lists for each
  // track type after v40.
  reg->beginGroup(s_grpSettings);
  auto value = reg->value(s_valMergePredefinedTrackNames);
  if (value.isValid()) {
    reg->remove(s_valMergePredefinedTrackNames);
    reg->setValue(s_valMergePredefinedVideoTrackNames,    value.toStringList());
    reg->setValue(s_valMergePredefinedAudioTrackNames,    value.toStringList());
    reg->setValue(s_valMergePredefinedSubtitleTrackNames, value.toStringList());
  }
  reg->endGroup();
}

void
convert46_0_0RunProgramConfigs(version_number_t const &writtenByVersion) {
  // Update program runner event types: v46 splits "run if job
  // successful or with warnings" into two separate events.
  if (writtenByVersion > version_number_t{"45.0.0.26"})
    return;

  auto reg = Settings::registry();

  reg->beginGroup(s_grpRunProgramConfigurations);

  for (auto const &group : reg->childGroups()) {
    reg->beginGroup(group);
    auto forEvents = reg->value(s_valForEvents).toInt();
    if (forEvents & 2)
      reg->setValue(s_valForEvents, forEvents | 8);
    reg->endGroup();
  }

  reg->endGroup();            // runProgramConfigurations
}

void
convert51_0_0OftenUsedCountries() {
  auto reg = Settings::registry();

  // "Often used countries" was renamed to "often used regions" in
  // v51.
  reg->beginGroup(s_grpSettings);
  if (!reg->value(s_valOftenUsedRegions).isValid()) {
    reg->setValue(s_valOftenUsedRegions, reg->value(s_valOftenUsedCountries));
    reg->remove(s_valOftenUsedCountries);
  }
  if (!reg->value(s_valOftenUsedRegionsOnly).isValid()) {
    reg->setValue(s_valOftenUsedRegionsOnly, reg->value(s_valOftenUsedCountriesOnly));
    reg->remove(s_valOftenUsedCountriesOnly);
  }
  reg->endGroup();
}

void
convert54_0_0AddingAppendingFilesPolicy() {
  auto reg = Settings::registry();

  // After v54: the setting mergeAddingAppendingFilesPolicy which was
  // only used for dragging & dropping in < v54 but for both dragging
  // & dropping and using the "add source files" button in v54 is
  // split up into two settings afterwards.
  reg->beginGroup(s_grpSettings);
  if (!reg->value(s_valMergeDragAndDropFilesPolicy).isValid()) {
    if (reg->value(s_valMergeAddingAppendingFilesPolicy).isValid())
      reg->setValue(s_valMergeDragAndDropFilesPolicy,       reg->value(s_valMergeAddingAppendingFilesPolicy));
    if (reg->value(s_valMergeLastAddingAppendingDecision).isValid())
      reg->setValue(s_valMergeLastDragAndDropFilesDecision, reg->value(s_valMergeLastAddingAppendingDecision));

    reg->remove(s_valMergeAddingAppendingFilesPolicy);
    reg->remove(s_valMergeLastAddingAppendingDecision);
  }
  reg->endGroup();
}

void
convert55_0_0DerivingTrackLanguagesFromFileNames() {
  auto reg = Settings::registry();

  // v55 uses boundary chars instead of a whole regex.
  reg->beginGroup(s_grpSettings);
  reg->beginGroup(s_grpDerivingTrackLanguagesFromFileNames);
  reg->remove("customRegex");
  reg->endGroup();
  reg->endGroup();
}

void
convert60_0_0DerivingTrackLanguagesBoundaryChars(version_number_t const &writtenByVersion) {
  // After v60: boundary characters for detecting track languages have changed.
  if (writtenByVersion > version_number_t{"60.0.0.2"})
    return;

  auto reg = Settings::registry();

  reg->beginGroup(s_grpSettings);
  reg->beginGroup(s_grpDerivingTrackLanguagesFromFileNames);
  if (reg->value(s_valBoundaryChars) == Q("[](){}.+=#"))
    reg->setValue(s_valBoundaryChars, Settings::defaultBoundaryCharsForDerivingLanguageFromFileName());
  reg->endGroup();
  reg->endGroup();
}

void
convert60_0_0ProcessPriority(version_number_t const &writtenByVersion) {
  // v60 changed default process priority to "lowest", which isn't a
  // good idea; 60.0.0.18 amended it to "low".
  if ((writtenByVersion < version_number_t{"60.0.0.0"}) || (writtenByVersion >= version_number_t{"60.0.0.18"}))
    return;

  auto reg = Settings::registry();

  reg->beginGroup(s_grpSettings);
  if (reg->value(s_valPriority).toInt() == static_cast<int>(Settings::LowestPriority))
    reg->setValue(s_valPriority, static_cast<int>(Settings::LowPriority));
  reg->endGroup();
}

void
convert66_0_0LanguageShortcuts() {
  auto reg = Settings::registry();

  reg->beginGroup(s_grpSettings);
  auto value = reg->value(s_valLanguageShortcuts);

  if (!value.isValid()) {
    reg->endGroup();
    return;
  }

  reg->remove(s_valLanguageShortcuts);
  reg->endGroup();

  reg->beginGroup(s_grpLanguageShortcuts);

  for (auto const &group : reg->childGroups())
    reg->remove(group);

  auto idx = 0;
  for (auto const &language : value.toStringList()) {
    reg->beginGroup(Q("%1").arg(++idx, 4, 10, Q('0')));
    reg->setValue(s_valLanguage,  language);
    reg->setValue(s_valTrackName, "");
    reg->endGroup();
  }

  reg->endGroup();               // languageShortcuts
}

void
convert67_0_0AttachmentsAlwaysSkipForExistingName(version_number_t const &writtenByVersion) {
  if (writtenByVersion >= version_number_t{"66.0.0.18"})
    return;

  auto reg = Settings::registry();

  reg->beginGroup(s_grpSettings);
  reg->setValue(s_valMergeAttachmentsAlwaysSkipForExistingName, false);
  reg->endGroup();
}

void
convert67_0_0UseISO639_3Languages(version_number_t const &writtenByVersion) {
  if (writtenByVersion >= version_number_t{"66.0.0.35"})
    return;

  auto reg = Settings::registry();

  reg->beginGroup(s_grpSettings);
  reg->setValue(s_valUseISO639_3Languages, true);
  reg->endGroup();
}

void
convert67_0_0DefaultAudioFileNames(version_number_t const &writtenByVersion) {
  if (writtenByVersion >= version_number_t{"67.0.0.0"})
    return;

  auto reg   = Settings::registry();
  auto oggRe = QRegularExpression{Q("<MTX_INSTALLATION_DIRECTORY>.*ogg$")};

  reg->beginGroup(s_grpRunProgramConfigurations);

  for (auto const &group : reg->childGroups()) {
    reg->beginGroup(group);
    auto audioFile = reg->value(s_valAudioFile).toString();

    if (audioFile.contains(oggRe))
      reg->setValue(s_valAudioFile, audioFile.left(audioFile.length() - 3) + Q("webm"));

    reg->endGroup();
  }

  reg->endGroup();            // runProgramConfigurations
}

} // anonymous namespace

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

  return m_type == RunProgramType::ExecuteProgram          ? nameForExternalProgram()
       : m_type == RunProgramType::PlayAudioFile           ? nameForPlayAudioFile()
       : m_type == RunProgramType::ShowDesktopNotification ? QY("Show a desktop notification")
       : m_type == RunProgramType::ShutDownComputer        ? QY("Shut down the computer")
       : m_type == RunProgramType::HibernateComputer       ? QY("Hibernate the computer")
       : m_type == RunProgramType::SleepComputer           ? QY("Sleep the computer")
       : m_type == RunProgramType::DeleteSourceFiles       ? QY("Delete source files for multiplexer jobs")
       : m_type == RunProgramType::QuitMKVToolNix          ? QY("Quit MKVToolNix")
       :                                                     Q("unknown");
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
  auto var = Q(mtx::sys::get_environment_variable("MKVTOOLNIX_GUI_STATE_DIR"));
  if (!var.isEmpty())
    return var;

#if defined(SYS_WINDOWS)
  if (!App::isInstalled())
    // QApplication::applicationDirPath() cannot be used here as the
    // Util::Settings class might be used before QCoreApplication's
    // been instantiated, which QApplication::applicationDirPath()
    // requires.
    return Q(mtx::sys::get_current_exe_path("").string());

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
  auto var = Q(mtx::sys::get_environment_variable("MKVTOOLNIX_GUI_CONFIG_FILE"));
  if (!var.isEmpty())
    return var;

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
  auto reg = std::make_unique<QSettings>(iniFileName(), QSettings::IniFormat);
  return reg;
}

void
Settings::convertOldSettings() {
  auto reg = registry();

  // Read the version number used for writing the settings.
  reg->beginGroup(s_grpInfo);
  auto writtenByVersion = version_number_t{to_utf8(reg->value(s_valGuiVersion).toString())};
  if (!writtenByVersion.valid)
    writtenByVersion = version_number_t{"8.1.0"}; // 8.1.0 was the last version not writing the version number field.
  reg->endGroup();

  convert8_1_0DefaultSubtitleCharset(writtenByVersion);
  convert8_1_0DefaultTrackLanguage();
  convert37_0_0OftenUsedLanguages(writtenByVersion);
  convert40_0_0PredefinedTrackNames();
  convert46_0_0RunProgramConfigs(writtenByVersion);
  convert51_0_0OftenUsedCountries();
  convert54_0_0AddingAppendingFilesPolicy();
  convert55_0_0DerivingTrackLanguagesFromFileNames();
  convert60_0_0DerivingTrackLanguagesBoundaryChars(writtenByVersion);
  convert60_0_0ProcessPriority(writtenByVersion);
  convert66_0_0LanguageShortcuts();
  convert67_0_0AttachmentsAlwaysSkipForExistingName(writtenByVersion);
  convert67_0_0UseISO639_3Languages(writtenByVersion);
  convert67_0_0DefaultAudioFileNames(writtenByVersion);
}

void
Settings::load() {
  convertOldSettings();

#if defined(SYS_APPLE)
  auto defaultElideTabHeaderLabels = true;
#else
  auto defaultElideTabHeaderLabels = false;
#endif

  auto regPtr      = registry();
  auto &reg        = *regPtr;
  auto defaultFont = defaultUiFont();
  std::optional<QVariant> enableMuxingTracksByTheseTypes;

  reg.beginGroup(s_grpSettings);
  m_priority                                  = static_cast<ProcessPriority>(reg.value(s_valPriority,                                          static_cast<int>(LowPriority)).toInt());
  m_probeRangePercentage                      = reg.value(s_valProbeRangePercentage,                                                           0.3).toDouble();
  m_tabPosition                               = static_cast<QTabWidget::TabPosition>(reg.value(s_valTabPosition,                               static_cast<int>(QTabWidget::North)).toInt());
  m_elideTabHeaderLabels                      = reg.value(s_valElideTabHeaderLabels,                                                           defaultElideTabHeaderLabels).toBool();
  m_useLegacyFontMIMETypes                    = reg.value(s_valUseLegacyFontMIMETypes,                                                         false).toBool();
  m_lastOpenDir                               = QDir{reg.value(s_valLastOpenDir).toString()};
  m_lastOutputDir                             = QDir{reg.value(s_valLastOutputDir).toString()};
  m_lastConfigDir                             = QDir{reg.value(s_valLastConfigDir).toString()};
  m_numRecentlyUsedStringsToRemember          = reg.value(s_valNumRecentlyUsedStringsToRemember,                                               10).toUInt();

  m_oftenUsedLanguages                        = reg.value(s_valOftenUsedLanguages).toStringList();
  m_oftenUsedRegions                          = reg.value(s_valOftenUsedRegions).toStringList();
  m_oftenUsedCharacterSets                    = reg.value(s_valOftenUsedCharacterSets).toStringList();

  m_oftenUsedLanguagesOnly                    = reg.value(s_valOftenUsedLanguagesOnly,                                                         false).toBool();
  m_oftenUsedRegionsOnly                      = reg.value(s_valOftenUsedRegionsOnly,                                                           false).toBool();
  m_oftenUsedCharacterSetsOnly                = reg.value(s_valOftenUsedCharacterSetsOnly,                                                     false).toBool();
  m_useISO639_3Languages                      = reg.value(s_valUseISO639_3Languages,                                                           true).toBool();

  m_scanForPlaylistsPolicy                    = static_cast<ScanForPlaylistsPolicy>(reg.value(s_valScanForPlaylistsPolicy,                     static_cast<int>(AskBeforeScanning)).toInt());
  m_minimumPlaylistDuration                   = reg.value(s_valMinimumPlaylistDuration,                                                        120).toUInt();
  m_ignorePlaylistsForMenus                   = reg.value(s_valIgnorePlaylistsForMenus,                                                        true).toBool();

  m_setAudioDelayFromFileName                 = reg.value(s_valSetAudioDelayFromFileName,                                                      true).toBool();
  m_autoSetFileTitle                          = reg.value(s_valAutoSetFileTitle,                                                               true).toBool();
  m_autoClearFileTitle                        = reg.value(s_valAutoClearFileTitle,                                                             m_autoSetFileTitle).toBool();
  m_clearMergeSettings                        = static_cast<ClearMergeSettingsAction>(reg.value(s_valClearMergeSettings,                       static_cast<int>(ClearMergeSettingsAction::None)).toInt());
  m_disableCompressionForAllTrackTypes        = reg.value(s_valDisableCompressionForAllTrackTypes,                                             false).toBool();
  m_disableDefaultTrackForSubtitles           = reg.value(s_valDisableDefaultTrackForSubtitles,                                                false).toBool();
  m_mergeEnableDialogNormGainRemoval          = reg.value(s_valMergeEnableDialogNormGainRemoval,                                               false).toBool();
  m_mergeAddBlurayCovers                      = reg.value(s_valMergeAddBlurayCovers,                                                           true).toBool();
  m_mergeAttachmentsAlwaysSkipForExistingName = reg.value(s_valMergeAttachmentsAlwaysSkipForExistingName,                                      true).toBool();
  m_mergeEnsureAtLeastOneTrackEnabled         = reg.value(s_valMergeEnsureAtLeastOneTrackEnabled,                                              true).toBool();
  m_mergeAlwaysCreateNewSettingsForVideoFiles = reg.value(s_valMergeAlwaysCreateNewSettingsForVideoFiles).toBool();
  m_mergeSortFilesTracksByTypeWhenAdding      = reg.value(s_valMergeSortFilesTracksByTypeWhenAdding,                                           true).toBool();
  m_mergeReconstructSequencesWhenAdding       = reg.value(s_valMergeReconstructSequencesWhenAdding,                                            true).toBool();
  m_mergeAlwaysShowOutputFileControls         = reg.value(s_valMergeAlwaysShowOutputFileControls,                                              true).toBool();
  m_mergeShowDNDZones                         = reg.value(s_valMergeShowDNDZones,                                                              true).toBool();
  m_mergePredefinedVideoTrackNames            = reg.value(s_valMergePredefinedVideoTrackNames).toStringList();
  m_mergePredefinedAudioTrackNames            = reg.value(s_valMergePredefinedAudioTrackNames).toStringList();
  m_mergePredefinedSubtitleTrackNames         = reg.value(s_valMergePredefinedSubtitleTrackNames).toStringList();
  m_mergePredefinedSplitSizes                 = reg.value(s_valMergePredefinedSplitSizes).toStringList();
  m_mergePredefinedSplitDurations             = reg.value(s_valMergePredefinedSplitDurations).toStringList();
  m_mergeDefaultCommandLineEscapeMode         = reg.value(s_valMergeDefaultCommandLineEscapeMode,                                              Merge::CommandLineDialog::platformDependentDefaultMode()).toInt();
  m_mergeTrackPropertiesLayout                = static_cast<TrackPropertiesLayout>(reg.value(s_valMergeTrackPropertiesLayout,                  static_cast<int>(TrackPropertiesLayout::HorizontalScrollArea)).toInt());
  m_mergeAddingAppendingFilesPolicy           = static_cast<MergeAddingAppendingFilesPolicy>(reg.value(s_valMergeAddingAppendingFilesPolicy,   static_cast<int>(MergeAddingAppendingFilesPolicy::Ask)).toInt());
  m_mergeLastAddingAppendingDecision          = static_cast<MergeAddingAppendingFilesPolicy>(reg.value(s_valMergeLastAddingAppendingDecision,  static_cast<int>(MergeAddingAppendingFilesPolicy::Add)).toInt());
  m_mergeDragAndDropFilesPolicy               = static_cast<MergeAddingAppendingFilesPolicy>(reg.value(s_valMergeDragAndDropFilesPolicy,       static_cast<int>(MergeAddingAppendingFilesPolicy::Ask)).toInt());
  m_mergeDragAndDropDirectoriesPolicy         = toMergeAddingDirectoriesPolicy(reg.value(s_valMergeDragAndDropDirectoriesPolicy,               static_cast<int>(MergeAddingDirectoriesPolicy::Ask)).toInt());
  m_mergeLastDragAndDropFilesDecision         = static_cast<MergeAddingAppendingFilesPolicy>(reg.value(s_valMergeLastDragAndDropFilesDecision, static_cast<int>(MergeAddingAppendingFilesPolicy::Add)).toInt());
  m_mergeWarnMissingAudioTrack                = static_cast<MergeMissingAudioTrackPolicy>(reg.value(s_valMergeWarnMissingAudioTrack,           static_cast<int>(MergeMissingAudioTrackPolicy::IfAudioTrackPresent)).toInt());
  m_headerEditorDroppedFilesPolicy            = static_cast<HeaderEditorDroppedFilesPolicy>(reg.value(s_valHeaderEditorDroppedFilesPolicy,     static_cast<int>(HeaderEditorDroppedFilesPolicy::Ask)).toInt());
  m_headerEditorDateTimeInUTC                 = reg.value(s_valHeaderEditorDateTimeInUTC).toBool();

  m_outputFileNamePolicy                      = static_cast<OutputFileNamePolicy>(reg.value(s_valOutputFileNamePolicy,                         static_cast<int>(ToSameAsFirstInputFile)).toInt());
  m_autoDestinationOnlyForVideoFiles          = reg.value(s_valAutoDestinationOnlyForVideoFiles,                                               false).toBool();
  m_mergeSetDestinationFromTitle              = reg.value(s_valMergeSetDestinationFromTitle,                                                   true).toBool();
  m_mergeSetDestinationFromDirectory          = reg.value(s_valMergeSetDestinationFromDirectory).toBool();
  m_relativeOutputDir                         = QDir{reg.value(s_valRelativeOutputDir).toString()};
  m_fixedOutputDir                            = QDir{reg.value(s_valFixedOutputDir).toString()};
  m_uniqueOutputFileNames                     = reg.value(s_valUniqueOutputFileNames,                                                          true).toBool();
  m_autoClearOutputFileName                   = reg.value(s_valAutoClearOutputFileName,                                                        m_outputFileNamePolicy != DontSetOutputFileName).toBool();

  m_enableMuxingTracksByLanguage              = reg.value(s_valEnableMuxingTracksByLanguage,                                                   false).toBool();
  m_enableMuxingAllVideoTracks                = reg.value(s_valEnableMuxingAllVideoTracks,                                                     true).toBool();
  m_enableMuxingAllAudioTracks                = reg.value(s_valEnableMuxingAllAudioTracks,                                                     false).toBool();
  m_enableMuxingAllSubtitleTracks             = reg.value(s_valEnableMuxingAllSubtitleTracks,                                                  false).toBool();
  m_enableMuxingForcedSubtitleTracks          = reg.value(s_valEnableMuxingForcedSubtitleTracks,                                               false).toBool();
  m_regexForRecognizingForcedSubtitleNames    = reg.value(s_valRegexForRecognizingForcedSubtitleNames,                                         Q("forced")).toString();
  m_enableMuxingTracksByTheseLanguages        = reg.value(s_valEnableMuxingTracksByTheseLanguages).toStringList();

  if (reg.contains("enableMuxingTracksByTheseTypes"))
    enableMuxingTracksByTheseTypes            = reg.value(s_valEnableMuxingTracksByTheseTypes);

  m_useDefaultJobDescription                  = reg.value(s_valUseDefaultJobDescription,                                                       false).toBool();
  m_showOutputOfAllJobs                       = reg.value(s_valShowOutputOfAllJobs,                                                            true).toBool();
  m_switchToJobOutputAfterStarting            = reg.value(s_valSwitchToJobOutputAfterStarting,                                                 false).toBool();
  m_resetJobWarningErrorCountersOnExit        = reg.value(s_valResetJobWarningErrorCountersOnExit,                                             false).toBool();
  m_removeOutputFileOnJobFailure              = reg.value(s_valRemoveOutputFileOnJobFailure,                                                   false).toBool();
  m_jobRemovalPolicy                          = static_cast<JobRemovalPolicy>(reg.value(s_valJobRemovalPolicy,                                 static_cast<int>(JobRemovalPolicy::Never)).toInt());
  m_jobRemovalOnExitPolicy                    = static_cast<JobRemovalPolicy>(reg.value(s_valJobRemovalOnExitPolicy,                           static_cast<int>(JobRemovalPolicy::Never)).toInt());
  m_maximumConcurrentJobs                     = reg.value(s_valMaximumConcurrentJobs,                                                          1).toUInt();
  m_removeOldJobs                             = reg.value(s_valRemoveOldJobs,                                                                  true).toBool();
  m_removeOldJobsDays                         = reg.value(s_valRemoveOldJobsDays,                                                              14).toInt();

  m_showToolSelector                          = reg.value(s_valShowToolSelector,                                                               true).toBool();
  m_warnBeforeClosingModifiedTabs             = reg.value(s_valWarnBeforeClosingModifiedTabs,                                                  true).toBool();
  m_warnBeforeAbortingJobs                    = reg.value(s_valWarnBeforeAbortingJobs,                                                         true).toBool();
  m_warnBeforeOverwriting                     = reg.value(s_valWarnBeforeOverwriting,                                                          true).toBool();
  m_showMoveUpDownButtons                     = reg.value(s_valShowMoveUpDownButtons,                                                          false).toBool();
  m_bcp47LanguageEditingMode                  = static_cast<BCP47LanguageEditingMode>(reg.value(s_valBCP47LanguageEditingMode,                 static_cast<int>(BCP47LanguageEditingMode::Components)).toInt());
  m_bcp47NormalizationMode                    = static_cast<mtx::bcp47::normalization_mode_e>(reg.value(s_valBCP47NormalizationMode,           static_cast<int>(mtx::bcp47::normalization_mode_e::default_mode)).toInt());

  m_chapterNameTemplate                       = reg.value(s_valChapterNameTemplate,                                                            QY("Chapter <NUM:2>")).toString();
  m_dropLastChapterFromBlurayPlaylist         = reg.value(s_valDropLastChapterFromBlurayPlaylist,                                              true).toBool();
  m_ceTextFileCharacterSet                    = reg.value(s_valCeTextFileCharacterSet).toString();

  m_mediaInfoExe                              = reg.value(s_valMediaInfoExe,                                                                   Q("mediainfo-gui")).toString();
  m_mediaInfoExe                              = determineMediaInfoExePath();

#if defined(HAVE_LIBINTL_H)
  m_uiLocale                                  = reg.value(s_valUiLocale).toString();
#endif
  m_uiDisableHighDPIScaling                   = reg.value(s_valUiDisableHighDPIScaling).toBool();
  m_uiDisableToolTips                         = reg.value(s_valUiDisableToolTips).toBool();
  m_uiFontFamily                              = reg.value(s_valUiFontFamily,                                                                   defaultFont.family()).toString();
  m_uiFontPointSize                           = reg.value(s_valUiFontPointSize,                                                                defaultFont.pointSize()).toInt();
  m_uiStayOnTop                               = reg.value(s_valUiStayOnTop,                                                                    false).toBool();
  m_uiPalette                                 = static_cast<AppPalette>(reg.value(s_valUiPalette,                                              static_cast<int>(AppPalette::OS)).toInt());

  m_showDebuggingMenu                         = reg.value(s_valShowDebuggingMenu,                                                              false).toBool();

  reg.beginGroup(s_grpUpdates);
  m_checkForUpdates                           = reg.value(s_valCheckForUpdates,                                                                true).toBool();
  m_lastUpdateCheck                           = reg.value(s_valLastUpdateCheck,                                                                QDateTime{}).toDateTime();

  reg.endGroup();               // settings.updates

  updateMaximumNumRecentlyUsedStrings();

  m_mergeLastFixedOutputDirs   .setItems(reg.value(s_valMergeLastFixedOutputDirs).toStringList());
  m_mergeLastOutputDirs        .setItems(reg.value(s_valMergeLastOutputDirs).toStringList());
  m_mergeLastRelativeOutputDirs.setItems(reg.value(s_valMergeLastRelativeOutputDirs).toStringList());

  reg.endGroup();               // settings

  loadDefaults(reg);
  loadDerivingTrackLanguagesSettings(reg);
  loadSplitterSizes(reg);
  loadDefaultInfoJobSettings(reg);
  loadFileColors(reg);
  loadRunProgramConfigurations(reg);
  loadLanguageShortcuts(reg);
  addDefaultRunProgramConfigurations(reg);
  setDefaults(enableMuxingTracksByTheseTypes);

  mtx::chapters::g_chapter_generation_name_template.override(to_utf8(m_chapterNameTemplate));
}

void
Settings::setDefaults(std::optional<QVariant> enableMuxingTracksByTheseTypes) {
  auto iso639UiLanguage = Q(translation_c::ms_default_iso639_ui_language);

  if (m_languageShortcuts.isEmpty()) {
    if (!iso639UiLanguage.isEmpty() && !m_oftenUsedLanguages.contains(iso639UiLanguage))
      m_languageShortcuts << LanguageShortcut{ iso639UiLanguage };

    m_languageShortcuts
      << LanguageShortcut { Q("en") }   // English
      << LanguageShortcut { Q("und") }  // undetermined
      << LanguageShortcut { Q("mul") }  // multiple languages
      << LanguageShortcut { Q("zxx") }; // no linguistic content
  }

  if (m_oftenUsedLanguages.isEmpty()) {
    m_oftenUsedLanguages
      << Q("mul")               // multiple languages
      << Q("zxx")               // no linguistic content
      << Q("qaa")               // reserved for local use
      << Q("mis")               // uncoded languages
      << Q("und")               // undetermined
      << Q("eng");              // English

    if (!iso639UiLanguage.isEmpty() && !m_oftenUsedLanguages.contains(iso639UiLanguage))
      m_oftenUsedLanguages << iso639UiLanguage;

    m_oftenUsedLanguages.sort();
  }

  if (m_oftenUsedCharacterSets.isEmpty())
    for (auto const &characterSet : g_popular_character_sets)
      m_oftenUsedCharacterSets << Q(characterSet);

  if (m_recognizedTrackLanguagesInFileNames.isEmpty())
    for (auto const &language : mtx::iso639::g_languages)
      if (!language.alpha_2_code.empty())
        m_recognizedTrackLanguagesInFileNames << Q(language.alpha_3_code);

  if (ToParentOfFirstInputFile == m_outputFileNamePolicy) {
    m_outputFileNamePolicy = ToRelativeOfFirstInputFile;
    m_relativeOutputDir.setPath(Q(".."));
  }

  m_enableMuxingTracksByTheseTypes.clear();
  if (enableMuxingTracksByTheseTypes)
    for (auto const &type : enableMuxingTracksByTheseTypes->toList())
      m_enableMuxingTracksByTheseTypes << static_cast<Merge::TrackType>(type.toInt());

  else
    for (int type = static_cast<int>(Merge::TrackType::Min); type <= static_cast<int>(Merge::TrackType::Max); ++type)
      m_enableMuxingTracksByTheseTypes << static_cast<Merge::TrackType>(type);

  if (m_mergePredefinedSplitSizes.isEmpty())
    m_mergePredefinedSplitSizes
      << Q("350M")
      << Q("650M")
      << Q("700M")
      << Q("703M")
      << Q("800M")
      << Q("1000M")
      << Q("4483M")
      << Q("8142M");

  if (m_mergePredefinedSplitDurations.isEmpty())
    m_mergePredefinedSplitDurations
      << Q("01:00:00")
      << Q("1800s");
}

void
Settings::loadDefaults(QSettings &reg) {
  reg.beginGroup(s_grpDefaults);
  m_defaultAudioTrackLanguage                        = mtx::bcp47::language_c::parse(to_utf8(reg.value(s_valDefaultAudioTrackLanguage,    Q("und")).toString()));
  m_defaultVideoTrackLanguage                        = mtx::bcp47::language_c::parse(to_utf8(reg.value(s_valDefaultVideoTrackLanguage,    Q("und")).toString()));
  m_defaultSubtitleTrackLanguage                     = mtx::bcp47::language_c::parse(to_utf8(reg.value(s_valDefaultSubtitleTrackLanguage, Q("und")).toString()));
  m_whenToSetDefaultLanguage                         = static_cast<SetDefaultLanguagePolicy>(reg.value(s_valWhenToSetDefaultLanguage,     static_cast<int>(SetDefaultLanguagePolicy::IfAbsentOrUndetermined)).toInt());
  m_defaultChapterLanguage                           = mtx::bcp47::language_c::parse(to_utf8(reg.value(s_valDefaultChapterLanguage,       Q("und")).toString()));
  m_defaultSetOriginalLanguageFlagLanguage           = mtx::bcp47::language_c::parse(to_utf8(reg.value(s_valDefaultSetOriginalLanguageFlagLanguage).toString()));
  m_defaultSubtitleCharset                           = reg.value(s_valDefaultSubtitleCharset).toString();
  m_defaultAdditionalMergeOptions                    = reg.value(s_valDefaultAdditionalMergeOptions).toString();
  m_deriveFlagsFromTrackNames                        = reg.value(s_valDefaultDeriveFlagsFromTrackNames,                                   true).toBool();
  m_deriveCommentaryFlagFromFileNames                = reg.value(s_valDefaultDeriveCommentaryFlagFromFileNames,                           true).toBool();
  m_regexForDerivingCommentaryFlagFromFileNames      = reg.value(s_valDefaultRegexForDerivingCommentaryFlagFromFileNames,                 defaultRegexForDerivingCommentaryFlagFromFileName()).toString();
  m_deriveHearingImpairedFlagFromFileNames           = reg.value(s_valDefaultDeriveHearingImpairedFlagFromFileNames,                      true).toBool();
  m_regexForDerivingHearingImpairedFlagFromFileNames = reg.value(s_valDefaultRegexForDerivingHearingImpairedFlagFromFileNames,            defaultRegexForDerivingHearingImpairedFlagFromFileName()).toString();
  m_deriveSubtitlesForcedFlagFromFileNames           = reg.value(s_valDefaultDeriveSubtitlesForcedFlagFromFileNames,                      true).toBool();
  m_regexForDerivingSubtitlesForcedFlagFromFileNames = reg.value(s_valDefaultRegexForDerivingSubtitlesForcedFlagFromFileNames,            defaultRegexForDerivingForcedDisplayFlagForSubtitlesFromFileName()).toString();
  reg.endGroup();               // defaults
}

void
Settings::loadDerivingTrackLanguagesSettings(QSettings &reg) {
  reg.beginGroup(s_grpSettings);
  reg.beginGroup(s_grpDerivingTrackLanguagesFromFileNames);

  m_deriveAudioTrackLanguageFromFileNamePolicy          = static_cast<DeriveLanguageFromFileNamePolicy>(reg.value(s_valAudioPolicy,    static_cast<int>(DeriveLanguageFromFileNamePolicy::IfAbsentOrUndetermined)).toInt());
  m_deriveVideoTrackLanguageFromFileNamePolicy          = static_cast<DeriveLanguageFromFileNamePolicy>(reg.value(s_valVideoPolicy,    static_cast<int>(DeriveLanguageFromFileNamePolicy::Never)).toInt());
  m_deriveSubtitleTrackLanguageFromFileNamePolicy       = static_cast<DeriveLanguageFromFileNamePolicy>(reg.value(s_valSubtitlePolicy, static_cast<int>(DeriveLanguageFromFileNamePolicy::IfAbsentOrUndetermined)).toInt());
  m_boundaryCharsForDerivingTrackLanguagesFromFileNames = reg.value(s_valBoundaryChars,                                                defaultBoundaryCharsForDerivingLanguageFromFileName()).toString();
  m_recognizedTrackLanguagesInFileNames                 = reg.value(s_valRecognizedTrackLanguagesInFileNames).toStringList();

  reg.endGroup();
  reg.endGroup();
}

void
Settings::loadSplitterSizes(QSettings &reg) {
  reg.beginGroup(s_grpSplitterSizes);

  m_splitterSizes.clear();
  for (auto const &name : reg.childKeys()) {
    auto sizes = reg.value(name).toList();
    for (auto const &size : sizes)
      m_splitterSizes[name] << size.toInt();
  }

  reg.endGroup();               // splitterSizes
}

void
Settings::loadDefaultInfoJobSettings(QSettings &reg) {
  reg.beginGroup(s_grpSettings);
  reg.beginGroup(s_grpInfo);
  reg.beginGroup(s_grpDefaultJobSettings);

  auto &s          = m_defaultInfoJobSettings;
  s.m_mode         = static_cast<Info::JobSettings::Mode>(     reg.value(s_valMode,      static_cast<int>(Info::JobSettings::Mode::Tree))                   .toInt());
  s.m_verbosity    = static_cast<Info::JobSettings::Verbosity>(reg.value(s_valVerbosity, static_cast<int>(Info::JobSettings::Verbosity::StopAtFirstCluster)).toInt());
  s.m_hexDumps     = static_cast<Info::JobSettings::HexDumps>( reg.value(s_valHexDumps,  static_cast<int>(Info::JobSettings::HexDumps::None))               .toInt());
  s.m_checksums    = reg.value(s_valChecksums).toBool();
  s.m_trackInfo    = reg.value(s_valTrackInfo).toBool();
  s.m_hexPositions = reg.value(s_valHexPositions).toBool();

  reg.endGroup();
  reg.endGroup();
  reg.endGroup();
}

void
Settings::loadRunProgramConfigurations(QSettings &reg) {
  m_runProgramConfigurations.clear();

  reg.beginGroup(s_grpRunProgramConfigurations);

  auto groups = reg.childGroups();
  groups.sort();

  for (auto const &group : groups) {
    auto cfg = std::make_shared<RunProgramConfig>();

    reg.beginGroup(group);
    cfg->m_active      = reg.value(s_valActive, true).toBool();
    cfg->m_name        = reg.value(s_valName).toString();
    auto type          = reg.value(s_valType, static_cast<int>(RunProgramType::ExecuteProgram)).toInt();
    cfg->m_type        = (type > static_cast<int>(RunProgramType::Min)) && (type < static_cast<int>(RunProgramType::Max)) ? static_cast<RunProgramType>(type) : RunProgramType::Default;
    cfg->m_forEvents   = static_cast<RunProgramForEvents>(reg.value(s_valForEvents).toInt());
    cfg->m_commandLine = reg.value(s_valCommandLine).toStringList();
    cfg->m_audioFile   = reg.value(s_valAudioFile).toString();
    cfg->m_volume      = std::min(reg.value(s_valVolume, 50).toUInt(), 100u);
    reg.endGroup();

    if (!cfg->m_active || cfg->isValid())
      m_runProgramConfigurations << cfg;
  }

  reg.endGroup();               // runProgramConfigurations
}

void
Settings::loadLanguageShortcuts(QSettings &reg) {
  m_languageShortcuts.clear();

  reg.beginGroup(s_grpLanguageShortcuts);

  auto groups = reg.childGroups();
  groups.sort();

  for (auto const &group : groups) {
    LanguageShortcut shortcut;

    reg.beginGroup(group);
    shortcut.m_language  = reg.value(s_valLanguage).toString();
    shortcut.m_trackName = reg.value(s_valTrackName).toString();
    reg.endGroup();

    m_languageShortcuts << shortcut;
  }

  reg.endGroup();               // languageShortcuts
}

void
Settings::loadFileColors(QSettings &reg) {
  reg.beginGroup(s_grpSettings);

  m_mergeUseFileAndTrackColors = reg.value(s_valMergeUseFileAndTrackColors, true).toBool();

  reg.beginGroup(s_grpFileColors);

  auto childKeys = reg.childKeys();
  mtx::sort::naturally(childKeys.begin(), childKeys.end());

  m_mergeFileColors.clear();
  m_mergeFileColors.reserve(childKeys.size());

  for (auto const &key : childKeys)
    m_mergeFileColors << reg.value(key).value<QColor>();

  reg.endGroup();               // fileColors
  reg.endGroup();               // settings

  if (m_mergeFileColors.isEmpty())
    m_mergeFileColors = defaultFileColors();
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
  reg.beginGroup(s_grpRunProgramConfigurations);

  auto numConfigurationsBefore = m_runProgramConfigurations.count();

  addDefaultRunProgramConfigurationForType(reg, RunProgramType::PlayAudioFile, [](RunProgramConfig &cfg) { cfg.m_audioFile = App::programRunner().defaultAudioFileName(); });
  addDefaultRunProgramConfigurationForType(reg, RunProgramType::ShowDesktopNotification, [](RunProgramConfig &cfg) { cfg.m_forEvents = RunAfterJobCompletesSuccessfully | RunAfterJobCompletesWithErrors | RunAfterJobCompletesWithWarnings; });
  addDefaultRunProgramConfigurationForType(reg, RunProgramType::SleepComputer);
  addDefaultRunProgramConfigurationForType(reg, RunProgramType::HibernateComputer);
  addDefaultRunProgramConfigurationForType(reg, RunProgramType::ShutDownComputer);
  addDefaultRunProgramConfigurationForType(reg, RunProgramType::DeleteSourceFiles, [](RunProgramConfig &cfg) { cfg.m_active = false; });
  addDefaultRunProgramConfigurationForType(reg, RunProgramType::QuitMKVToolNix);

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
Settings::updateMaximumNumRecentlyUsedStrings() {
  m_mergeLastFixedOutputDirs   .setMaximumNumItems(m_numRecentlyUsedStringsToRemember);
  m_mergeLastOutputDirs        .setMaximumNumItems(m_numRecentlyUsedStringsToRemember);
  m_mergeLastRelativeOutputDirs.setMaximumNumItems(m_numRecentlyUsedStringsToRemember);
}

void
Settings::save()
  const {
  auto regPtr = registry();
  auto &reg   = *regPtr;

  QVariantList enableMuxingTracksByTheseTypes;
  for (auto type : m_enableMuxingTracksByTheseTypes)
    enableMuxingTracksByTheseTypes << static_cast<int>(type);

  reg.beginGroup(s_grpInfo);
  reg.setValue(s_valGuiVersion,                                Q(get_current_version().to_string()));
  reg.endGroup();

  reg.beginGroup(s_grpSettings);
  reg.setValue(s_valPriority,                                  static_cast<int>(m_priority));
  reg.setValue(s_valProbeRangePercentage,                      m_probeRangePercentage);
  reg.setValue(s_valTabPosition,                               static_cast<int>(m_tabPosition));
  reg.setValue(s_valElideTabHeaderLabels,                      m_elideTabHeaderLabels);
  reg.setValue(s_valUseLegacyFontMIMETypes,                    m_useLegacyFontMIMETypes);
  reg.setValue(s_valLastOpenDir,                               m_lastOpenDir.path());
  reg.setValue(s_valLastOutputDir,                             m_lastOutputDir.path());
  reg.setValue(s_valLastConfigDir,                             m_lastConfigDir.path());
  reg.setValue(s_valNumRecentlyUsedStringsToRemember,          m_numRecentlyUsedStringsToRemember);

  reg.setValue(s_valOftenUsedLanguages,                        m_oftenUsedLanguages);
  reg.setValue(s_valOftenUsedRegions,                          m_oftenUsedRegions);
  reg.setValue(s_valOftenUsedCharacterSets,                    m_oftenUsedCharacterSets);

  reg.setValue(s_valOftenUsedLanguagesOnly,                    m_oftenUsedLanguagesOnly);
  reg.setValue(s_valOftenUsedRegionsOnly,                      m_oftenUsedRegionsOnly);
  reg.setValue(s_valOftenUsedCharacterSetsOnly,                m_oftenUsedCharacterSetsOnly);
  reg.setValue(s_valUseISO639_3Languages,                      m_useISO639_3Languages);

  reg.setValue(s_valScanForPlaylistsPolicy,                    static_cast<int>(m_scanForPlaylistsPolicy));
  reg.setValue(s_valMinimumPlaylistDuration,                   m_minimumPlaylistDuration);
  reg.setValue(s_valIgnorePlaylistsForMenus,                   m_ignorePlaylistsForMenus);

  reg.setValue(s_valSetAudioDelayFromFileName,                 m_setAudioDelayFromFileName);
  reg.setValue(s_valAutoSetFileTitle,                          m_autoSetFileTitle);
  reg.setValue(s_valAutoClearFileTitle,                        m_autoClearFileTitle);
  reg.setValue(s_valClearMergeSettings,                        static_cast<int>(m_clearMergeSettings));
  reg.setValue(s_valDisableCompressionForAllTrackTypes,        m_disableCompressionForAllTrackTypes);
  reg.setValue(s_valDisableDefaultTrackForSubtitles,           m_disableDefaultTrackForSubtitles);
  reg.setValue(s_valMergeEnableDialogNormGainRemoval,          m_mergeEnableDialogNormGainRemoval);
  reg.setValue(s_valMergeAddBlurayCovers,                      m_mergeAddBlurayCovers);
  reg.setValue(s_valMergeAttachmentsAlwaysSkipForExistingName, m_mergeAttachmentsAlwaysSkipForExistingName);
  reg.setValue(s_valMergeEnsureAtLeastOneTrackEnabled,         m_mergeEnsureAtLeastOneTrackEnabled);
  reg.setValue(s_valMergeAlwaysCreateNewSettingsForVideoFiles, m_mergeAlwaysCreateNewSettingsForVideoFiles);
  reg.setValue(s_valMergeSortFilesTracksByTypeWhenAdding,      m_mergeSortFilesTracksByTypeWhenAdding);
  reg.setValue(s_valMergeReconstructSequencesWhenAdding,       m_mergeReconstructSequencesWhenAdding);
  reg.setValue(s_valMergeAlwaysShowOutputFileControls,         m_mergeAlwaysShowOutputFileControls);
  reg.setValue(s_valMergeShowDNDZones,                         m_mergeShowDNDZones);
  reg.setValue(s_valMergePredefinedVideoTrackNames,            m_mergePredefinedVideoTrackNames);
  reg.setValue(s_valMergePredefinedAudioTrackNames,            m_mergePredefinedAudioTrackNames);
  reg.setValue(s_valMergePredefinedSubtitleTrackNames,         m_mergePredefinedSubtitleTrackNames);
  reg.setValue(s_valMergePredefinedSplitSizes,                 m_mergePredefinedSplitSizes);
  reg.setValue(s_valMergePredefinedSplitDurations,             m_mergePredefinedSplitDurations);
  reg.setValue(s_valMergeLastFixedOutputDirs,                  m_mergeLastFixedOutputDirs.items());
  reg.setValue(s_valMergeLastOutputDirs,                       m_mergeLastOutputDirs.items());
  reg.setValue(s_valMergeLastRelativeOutputDirs,               m_mergeLastRelativeOutputDirs.items());
  reg.setValue(s_valMergeDefaultCommandLineEscapeMode,         m_mergeDefaultCommandLineEscapeMode);
  reg.setValue(s_valMergeTrackPropertiesLayout,                static_cast<int>(m_mergeTrackPropertiesLayout));
  reg.setValue(s_valMergeAddingAppendingFilesPolicy,           static_cast<int>(m_mergeAddingAppendingFilesPolicy));
  reg.setValue(s_valMergeLastAddingAppendingDecision,          static_cast<int>(m_mergeLastAddingAppendingDecision));
  reg.setValue(s_valMergeDragAndDropFilesPolicy,               static_cast<int>(m_mergeDragAndDropFilesPolicy));
  reg.setValue(s_valMergeDragAndDropDirectoriesPolicy,         static_cast<int>(m_mergeDragAndDropDirectoriesPolicy));
  reg.setValue(s_valMergeLastDragAndDropFilesDecision,         static_cast<int>(m_mergeLastDragAndDropFilesDecision));
  reg.setValue(s_valMergeWarnMissingAudioTrack,                static_cast<int>(m_mergeWarnMissingAudioTrack));
  reg.setValue(s_valHeaderEditorDroppedFilesPolicy,            static_cast<int>(m_headerEditorDroppedFilesPolicy));
  reg.setValue(s_valHeaderEditorDateTimeInUTC,                 m_headerEditorDateTimeInUTC);

  reg.setValue(s_valOutputFileNamePolicy,                      static_cast<int>(m_outputFileNamePolicy));
  reg.setValue(s_valAutoDestinationOnlyForVideoFiles,          m_autoDestinationOnlyForVideoFiles);
  reg.setValue(s_valMergeSetDestinationFromTitle,              m_mergeSetDestinationFromTitle);
  reg.setValue(s_valMergeSetDestinationFromDirectory,          m_mergeSetDestinationFromDirectory);
  reg.setValue(s_valRelativeOutputDir,                         m_relativeOutputDir.path());
  reg.setValue(s_valFixedOutputDir,                            m_fixedOutputDir.path());
  reg.setValue(s_valUniqueOutputFileNames,                     m_uniqueOutputFileNames);
  reg.setValue(s_valAutoClearOutputFileName,                   m_autoClearOutputFileName);

  reg.setValue(s_valEnableMuxingTracksByLanguage,              m_enableMuxingTracksByLanguage);
  reg.setValue(s_valEnableMuxingAllVideoTracks,                m_enableMuxingAllVideoTracks);
  reg.setValue(s_valEnableMuxingAllAudioTracks,                m_enableMuxingAllAudioTracks);
  reg.setValue(s_valEnableMuxingAllSubtitleTracks,             m_enableMuxingAllSubtitleTracks);
  reg.setValue(s_valEnableMuxingForcedSubtitleTracks,          m_enableMuxingForcedSubtitleTracks);
  reg.setValue(s_valRegexForRecognizingForcedSubtitleNames,    m_regexForRecognizingForcedSubtitleNames);
  reg.setValue(s_valEnableMuxingTracksByTheseLanguages,        m_enableMuxingTracksByTheseLanguages);
  reg.setValue(s_valEnableMuxingTracksByTheseTypes,            enableMuxingTracksByTheseTypes);

  reg.setValue(s_valUseDefaultJobDescription,                  m_useDefaultJobDescription);
  reg.setValue(s_valShowOutputOfAllJobs,                       m_showOutputOfAllJobs);
  reg.setValue(s_valSwitchToJobOutputAfterStarting,            m_switchToJobOutputAfterStarting);
  reg.setValue(s_valResetJobWarningErrorCountersOnExit,        m_resetJobWarningErrorCountersOnExit);
  reg.setValue(s_valRemoveOutputFileOnJobFailure,              m_removeOutputFileOnJobFailure);
  reg.setValue(s_valJobRemovalPolicy,                          static_cast<int>(m_jobRemovalPolicy));
  reg.setValue(s_valJobRemovalOnExitPolicy,                    static_cast<int>(m_jobRemovalOnExitPolicy));
  reg.setValue(s_valMaximumConcurrentJobs,                     m_maximumConcurrentJobs);
  reg.setValue(s_valRemoveOldJobs,                             m_removeOldJobs);
  reg.setValue(s_valRemoveOldJobsDays,                         m_removeOldJobsDays);

  reg.setValue(s_valShowToolSelector,                          m_showToolSelector);
  reg.setValue(s_valWarnBeforeClosingModifiedTabs,             m_warnBeforeClosingModifiedTabs);
  reg.setValue(s_valWarnBeforeAbortingJobs,                    m_warnBeforeAbortingJobs);
  reg.setValue(s_valWarnBeforeOverwriting,                     m_warnBeforeOverwriting);
  reg.setValue(s_valShowMoveUpDownButtons,                     m_showMoveUpDownButtons);
  reg.setValue(s_valBCP47LanguageEditingMode,                  static_cast<int>(m_bcp47LanguageEditingMode));
  reg.setValue(s_valBCP47NormalizationMode,                    static_cast<int>(m_bcp47NormalizationMode));

  reg.setValue(s_valChapterNameTemplate,                       m_chapterNameTemplate);
  reg.setValue(s_valDropLastChapterFromBlurayPlaylist,         m_dropLastChapterFromBlurayPlaylist);
  reg.setValue(s_valCeTextFileCharacterSet,                    m_ceTextFileCharacterSet);

  reg.setValue(s_valUiLocale,                                  m_uiLocale);
  reg.setValue(s_valUiDisableHighDPIScaling,                   m_uiDisableHighDPIScaling);
  reg.setValue(s_valUiDisableToolTips,                         m_uiDisableToolTips);
  reg.setValue(s_valUiFontFamily,                              m_uiFontFamily);
  reg.setValue(s_valUiFontPointSize,                           m_uiFontPointSize);
  reg.setValue(s_valUiStayOnTop,                               m_uiStayOnTop);
  reg.setValue(s_valUiPalette,                                 static_cast<int>(m_uiPalette));

  reg.setValue(s_valMediaInfoExe,                              m_mediaInfoExe);

  reg.setValue(s_valShowDebuggingMenu,                         m_showDebuggingMenu);

  reg.beginGroup(s_grpUpdates);
  reg.setValue(s_valCheckForUpdates,                           m_checkForUpdates);
  reg.setValue(s_valLastUpdateCheck,                           m_lastUpdateCheck);
  reg.endGroup();               // settings.updates
  reg.endGroup();               // settings

  saveDefaults(reg);
  saveDerivingTrackLanguagesSettings(reg);
  saveSplitterSizes(reg);
  saveDefaultInfoJobSettings(reg);
  saveFileColors(reg);
  saveRunProgramConfigurations(reg);
  saveLanguageShortcuts(reg);
}

void
Settings::saveDefaults(QSettings &reg)
  const {
  reg.beginGroup(s_grpDefaults);
  reg.setValue(s_valDefaultAudioTrackLanguage,                               Q(m_defaultAudioTrackLanguage.format()));
  reg.setValue(s_valDefaultVideoTrackLanguage,                               Q(m_defaultVideoTrackLanguage.format()));
  reg.setValue(s_valDefaultSubtitleTrackLanguage,                            Q(m_defaultSubtitleTrackLanguage.format()));
  reg.setValue(s_valWhenToSetDefaultLanguage,                                static_cast<int>(m_whenToSetDefaultLanguage));
  reg.setValue(s_valDefaultChapterLanguage,                                  Q(m_defaultChapterLanguage.format()));
  reg.setValue(s_valDefaultSetOriginalLanguageFlagLanguage,                  Q(m_defaultSetOriginalLanguageFlagLanguage.format()));
  reg.setValue(s_valDefaultSubtitleCharset,                                  m_defaultSubtitleCharset);
  reg.setValue(s_valDefaultAdditionalMergeOptions,                           m_defaultAdditionalMergeOptions);
  reg.setValue(s_valDefaultDeriveFlagsFromTrackNames,                        m_deriveFlagsFromTrackNames);
  reg.setValue(s_valDefaultDeriveCommentaryFlagFromFileNames,                m_deriveCommentaryFlagFromFileNames);
  reg.setValue(s_valDefaultRegexForDerivingCommentaryFlagFromFileNames,      m_regexForDerivingCommentaryFlagFromFileNames);
  reg.setValue(s_valDefaultDeriveHearingImpairedFlagFromFileNames,           m_deriveHearingImpairedFlagFromFileNames);
  reg.setValue(s_valDefaultRegexForDerivingHearingImpairedFlagFromFileNames, m_regexForDerivingHearingImpairedFlagFromFileNames);
  reg.setValue(s_valDefaultDeriveSubtitlesForcedFlagFromFileNames,           m_deriveSubtitlesForcedFlagFromFileNames);
  reg.setValue(s_valDefaultRegexForDerivingSubtitlesForcedFlagFromFileNames, m_regexForDerivingSubtitlesForcedFlagFromFileNames);
  reg.endGroup();               // defaults
}

void
Settings::saveDerivingTrackLanguagesSettings(QSettings &reg)
  const {
  reg.beginGroup(s_grpSettings);
  reg.beginGroup(s_grpDerivingTrackLanguagesFromFileNames);

  reg.setValue(s_valAudioPolicy,                         static_cast<int>(m_deriveAudioTrackLanguageFromFileNamePolicy));
  reg.setValue(s_valVideoPolicy,                         static_cast<int>(m_deriveVideoTrackLanguageFromFileNamePolicy));
  reg.setValue(s_valSubtitlePolicy,                      static_cast<int>(m_deriveSubtitleTrackLanguageFromFileNamePolicy));
  reg.setValue(s_valBoundaryChars,                       m_boundaryCharsForDerivingTrackLanguagesFromFileNames);
  reg.setValue(s_valRecognizedTrackLanguagesInFileNames, m_recognizedTrackLanguagesInFileNames);

  reg.endGroup();
  reg.endGroup();
}

void
Settings::saveSplitterSizes(QSettings &reg)
  const {
  reg.beginGroup(s_grpSplitterSizes);
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
Settings::saveDefaultInfoJobSettings(QSettings &reg)
  const {
  reg.beginGroup(s_grpSettings);
  reg.beginGroup(s_grpInfo);
  reg.beginGroup(s_grpDefaultJobSettings);

  auto &s = m_defaultInfoJobSettings;
  reg.setValue(s_valMode,         static_cast<int>(s.m_mode));
  reg.setValue(s_valVerbosity,    static_cast<int>(s.m_verbosity));
  reg.setValue(s_valHexDumps,     static_cast<int>(s.m_hexDumps));
  reg.setValue(s_valChecksums,    s.m_checksums);
  reg.setValue(s_valTrackInfo,    s.m_trackInfo);
  reg.setValue(s_valHexPositions, s.m_hexPositions);

  reg.endGroup();
  reg.endGroup();
  reg.endGroup();
}

void
Settings::saveRunProgramConfigurations(QSettings &reg)
  const {
  reg.beginGroup(s_grpRunProgramConfigurations);

  auto groups = reg.childGroups();
  groups.sort();

  for (auto const &group : groups)
    reg.remove(group);

  auto idx = 0;
  for (auto const &cfg : m_runProgramConfigurations) {
    reg.beginGroup(Q("%1").arg(++idx, 4, 10, Q('0')));
    reg.setValue(s_valActive,      cfg->m_active);
    reg.setValue(s_valName,        cfg->m_name);
    reg.setValue(s_valType,        static_cast<int>(cfg->m_type));
    reg.setValue(s_valForEvents,   static_cast<int>(cfg->m_forEvents));
    reg.setValue(s_valCommandLine, cfg->m_commandLine);
    reg.setValue(s_valAudioFile,   cfg->m_audioFile);
    reg.setValue(s_valVolume,      cfg->m_volume);
    reg.endGroup();
  }

  reg.endGroup();               // runProgramConfigurations
}

void
Settings::saveLanguageShortcuts(QSettings &reg)
  const {
  reg.beginGroup(s_grpLanguageShortcuts);

  for (auto const &group : reg.childGroups())
    reg.remove(group);

  auto idx = 0;
  for (auto const &shortcut : m_languageShortcuts) {
    reg.beginGroup(Q("%1").arg(++idx, 4, 10, Q('0')));
    reg.setValue(s_valLanguage,  shortcut.m_language);
    reg.setValue(s_valTrackName, shortcut.m_trackName);
    reg.endGroup();
  }

  reg.endGroup();               // languageShortcuts
}

void
Settings::saveFileColors(QSettings &reg)
  const {
  reg.beginGroup(s_grpSettings);

  reg.setValue(s_valMergeUseFileAndTrackColors, m_mergeUseFileAndTrackColors);

  reg.remove(s_grpFileColors);

  reg.beginGroup(s_grpFileColors);

  auto idx = 0;
  for (auto const &color : m_mergeFileColors)
    reg.setValue(Q("color%1").arg(idx++), color);

  reg.endGroup();               // fileColors
  reg.endGroup();               // settings
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
  auto path          = mtx::fs::to_path( to_utf8(exe) );
  auto program       = path.filename();
  auto installPath   = mtx::fs::to_path( to_utf8(App::applicationDirPath()) );
  auto potentialExes = QList<boost::filesystem::path>{} << path << (installPath / path) << (installPath / ".." / path);

#if defined(SYS_WINDOWS)
  for (auto &potentialExe : potentialExes)
    potentialExe.replace_extension(mtx::fs::to_path("exe"));

  program.replace_extension(mtx::fs::to_path("exe"));
#endif  // SYS_WINDOWS

  for (auto const &potentialExe : potentialExes) {
    [[maybe_unused]] boost::system::error_code ec;
    if (boost::filesystem::is_regular_file(potentialExe, ec))
      return QDir::toNativeSeparators(to_qs(potentialExe.string()));
  }

  auto location = QStandardPaths::findExecutable(to_qs(program.string()));
  if (!location.isEmpty())
    return QDir::toNativeSeparators(location);

  return QDir::toNativeSeparators(exe);
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
    locale = to_utf8(Q(translation_c::get_default_ui_locale()).replace(QRegularExpression{"\\..*"}, {}));
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

QString
Settings::determineMediaInfoExePath() {
  auto &cfg          = get();
  auto potentialExes = QStringList{exeWithPath(cfg.m_mediaInfoExe.isEmpty() ? Q("mediainfo-gui") : cfg.m_mediaInfoExe)};

#if defined(SYS_WINDOWS)
  potentialExes << QSettings{Q("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\MediaInfo.exe"), QSettings::NativeFormat}.value("Default").toString();
#endif

  potentialExes << exeWithPath(Q("mediainfo"));

  for (auto const &exe : potentialExes)
    if (!exe.isEmpty() && QFileInfo{exe}.exists())
      return QDir::toNativeSeparators(exe);

  return {};
}

QString
Settings::defaultBoundaryCharsForDerivingLanguageFromFileName() {
  return Q("[](){}.+-=#");
}

QString
Settings::defaultRegexForDerivingHearingImpairedFlagFromFileName() {
  return Q(R"((^|[[\](){} .+=#-])(cc|sdh)([[\](){} .+=#-]|$))");
}

QString
Settings::defaultRegexForDerivingForcedDisplayFlagForSubtitlesFromFileName() {
  return Q(R"((^|[[\](){}.+=#-])forced([[\](){}.+=#-]|$))");
}

QString
Settings::defaultRegexForDerivingCommentaryFlagFromFileName() {
  return Q(R"((^|[[\](){} .+=#-])(comments|commentary)([[\](){} .+=#-]|$))");
}

QVector<QColor>
Settings::defaultFileColors() {
  QVector<QColor> colors;

  auto step = 64;

  colors.clear();
  colors.reserve(6 * (256 / step));

  for (int value = 255; value > step; value -= step) {
    colors << QColor{0,            value,        0};
    colors << QColor{0,            0,            value};
    colors << QColor{value,        0,            0};
    colors << QColor{value,        value,        0};
    colors << QColor{value,        0,            value};
    colors << QColor{0,            value,        value};
    colors << QColor{value - step, value - step, value - step};
  }

  return colors;
}

QColor
Settings::nthFileColor(int idx)
  const {
  static QVector<QColor> s_additionalColors;

  if (idx < 0)
    return {};

  if (idx < m_mergeFileColors.size())
    return m_mergeFileColors.at(idx);

  idx -= m_mergeFileColors.size();

  while (idx >= s_additionalColors.size())
    s_additionalColors << QColor{random_c::generate_8bits(), random_c::generate_8bits(), random_c::generate_8bits()};

  return s_additionalColors.at(idx);
}

Settings::MergeAddingDirectoriesPolicy
Settings::toMergeAddingDirectoriesPolicy(int value) {
  return value == static_cast<int>(MergeAddingDirectoriesPolicy::Flat)                  ? MergeAddingDirectoriesPolicy::Flat
       : value == static_cast<int>(MergeAddingDirectoriesPolicy::AddEachDirectoryToNew) ? MergeAddingDirectoriesPolicy::AddEachDirectoryToNew
       :                                                                                  MergeAddingDirectoriesPolicy::Ask;
}

}
