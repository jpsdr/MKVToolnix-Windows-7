#include "common/common_pch.h"

#include "common/extern_data.h"
#include "common/fs_sys_helpers.h"
#include "common/iso639.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/util/settings.h"

#include <QSettings>

namespace mtx { namespace gui { namespace Util {

Settings Settings::s_settings;

Settings::Settings()
{
  load();
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

std::unique_ptr<QSettings>
Settings::registry() {
#if defined(SYS_WINDOWS)
  if (!App::isInstalled())
    return std::make_unique<QSettings>(Q((mtx::sys::get_installation_path() / "mkvtoolnix-gui.ini").string()), QSettings::IniFormat);
#endif
  return std::make_unique<QSettings>();
}

void
Settings::load() {
  auto regPtr = registry();
  auto &reg   = *regPtr;

  reg.beginGroup("settings");
  m_priority                           = static_cast<ProcessPriority>(reg.value("priority", static_cast<int>(NormalPriority)).toInt());
  m_lastOpenDir                        = QDir{reg.value("lastOpenDir").toString()};
  m_lastOutputDir                      = QDir{reg.value("lastOutputDir").toString()};
  m_lastConfigDir                      = QDir{reg.value("lastConfigDir").toString()};
  m_lastMatroskaFileDir                = QDir{reg.value("lastMatroskaFileDir").toString()};

  m_oftenUsedLanguages                 = reg.value("oftenUsedLanguages").toStringList();
  m_oftenUsedCountries                 = reg.value("oftenUsedCountries").toStringList();
  m_oftenUsedCharacterSets             = reg.value("oftenUsedCharacterSets").toStringList();

  m_scanForPlaylistsPolicy             = static_cast<ScanForPlaylistsPolicy>(reg.value("scanForPlaylistsPolicy", static_cast<int>(AskBeforeScanning)).toInt());
  m_minimumPlaylistDuration            = reg.value("minimumPlaylistDuration", 120).toUInt();

  m_setAudioDelayFromFileName          = reg.value("setAudioDelayFromFileName", true).toBool();
  m_autoSetFileTitle                   = reg.value("autoSetFileTitle",          true).toBool();
  m_clearMergeSettings                 = static_cast<ClearMergeSettingsAction>(reg.value("clearMergeSettings", static_cast<int>(ClearMergeSettingsAction::None)).toInt());
  m_disableCompressionForAllTrackTypes = reg.value("disableCompressionForAllTrackTypes", false).toBool();

  m_uniqueOutputFileNames              = reg.value("uniqueOutputFileNames",     true).toBool();
  m_outputFileNamePolicy               = static_cast<OutputFileNamePolicy>(reg.value("outputFileNamePolicy", static_cast<int>(ToSameAsFirstInputFile)).toInt());
  m_fixedOutputDir                     = QDir{reg.value("fixedOutputDir").toString()};

  m_enableMuxingTracksByLanguage       = reg.value("enableMuxingTracksByLanguage", false).toBool();
  m_enableMuxingAllVideoTracks         = reg.value("enableMuxingAllVideoTracks", true).toBool();
  m_enableMuxingAllAudioTracks         = reg.value("enableMuxingAllAudioTracks", false).toBool();
  m_enableMuxingAllSubtitleTracks      = reg.value("enableMuxingAllSubtitleTracks", false).toBool();
  m_enableMuxingTracksByTheseLanguages = reg.value("enableMuxingTracksByTheseLanguages").toStringList();

  m_jobRemovalPolicy                   = static_cast<JobRemovalPolicy>(reg.value("jobRemovalPolicy", static_cast<int>(JobRemovalPolicy::Never)).toInt());

  m_disableAnimations                  = reg.value("disableAnimations", false).toBool();
  m_warnBeforeClosingModifiedTabs      = reg.value("warnBeforeClosingModifiedTabs", true).toBool();
  m_warnBeforeAbortingJobs             = reg.value("warnBeforeAbortingJobs", true).toBool();

#if defined(HAVE_LIBINTL_H)
  m_uiLocale                           = reg.value("uiLocale").toString();
#endif

  reg.beginGroup("updates");
  m_checkForUpdates                    = reg.value("checkForUpdates", true).toBool();
  m_lastUpdateCheck                    = reg.value("lastUpdateCheck", QDateTime{}).toDateTime();
  reg.endGroup();               // settings.updates
  reg.endGroup();               // settings

  reg.beginGroup("defaults");
  m_defaultTrackLanguage               = reg.value("defaultTrackLanguage", Q("und")).toString();
  m_defaultChapterLanguage             = reg.value("defaultChapterLanguage", Q("und")).toString();
  m_defaultChapterCountry              = reg.value("defaultChapterCountry").toString();
  m_defaultSubtitleCharset             = reg.value("defaultSubtitleCharset").toString();
  m_defaultAdditionalMergeOptions      = reg.value("defaultAdditionalMergeOptions").toString();
  reg.endGroup();               // defaults

  if (m_oftenUsedLanguages.isEmpty())
    for (auto const &languageCode : g_popular_language_codes)
      m_oftenUsedLanguages << Q(languageCode);

  if (m_oftenUsedCountries.isEmpty())
    for (auto const &countryCode : g_popular_country_codes)
      m_oftenUsedCountries << Q(countryCode);

  if (m_oftenUsedCharacterSets.isEmpty())
    for (auto const &characterSet : g_popular_character_sets)
      m_oftenUsedCharacterSets << Q(characterSet);
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

  reg.beginGroup("settings");
  reg.setValue("priority",                           static_cast<int>(m_priority));
  reg.setValue("lastOpenDir",                        m_lastOpenDir.path());
  reg.setValue("lastOutputDir",                      m_lastOutputDir.path());
  reg.setValue("lastConfigDir",                      m_lastConfigDir.path());
  reg.setValue("lastMatroskaFileDir",                m_lastMatroskaFileDir.path());

  reg.setValue("oftenUsedLanguages",                 m_oftenUsedLanguages);
  reg.setValue("oftenUsedCountries",                 m_oftenUsedCountries);
  reg.setValue("oftenUsedCharacterSets",             m_oftenUsedCharacterSets);

  reg.setValue("scanForPlaylistsPolicy",             static_cast<int>(m_scanForPlaylistsPolicy));
  reg.setValue("minimumPlaylistDuration",            m_minimumPlaylistDuration);

  reg.setValue("setAudioDelayFromFileName",          m_setAudioDelayFromFileName);
  reg.setValue("autoSetFileTitle",                   m_autoSetFileTitle);
  reg.setValue("clearMergeSettings",                 static_cast<int>(m_clearMergeSettings));
  reg.setValue("disableCompressionForAllTrackTypes", m_disableCompressionForAllTrackTypes);

  reg.setValue("outputFileNamePolicy",               static_cast<int>(m_outputFileNamePolicy));
  reg.setValue("fixedOutputDir",                     m_fixedOutputDir.path());
  reg.setValue("uniqueOutputFileNames",              m_uniqueOutputFileNames);

  reg.setValue("enableMuxingTracksByLanguage",       m_enableMuxingTracksByLanguage);
  reg.setValue("enableMuxingAllVideoTracks",         m_enableMuxingAllVideoTracks);
  reg.setValue("enableMuxingAllAudioTracks",         m_enableMuxingAllAudioTracks);
  reg.setValue("enableMuxingAllSubtitleTracks",      m_enableMuxingAllSubtitleTracks);
  reg.setValue("enableMuxingTracksByTheseLanguages", m_enableMuxingTracksByTheseLanguages);

  reg.setValue("jobRemovalPolicy",                   static_cast<int>(m_jobRemovalPolicy));

  reg.setValue("disableAnimations",                  m_disableAnimations);
  reg.setValue("warnBeforeClosingModifiedTabs",      m_warnBeforeClosingModifiedTabs);
  reg.setValue("warnBeforeAbortingJobs",             m_warnBeforeAbortingJobs);

  reg.setValue("uiLocale",                           m_uiLocale);

  reg.beginGroup("updates");
  reg.setValue("checkForUpdates",                    m_checkForUpdates);
  reg.setValue("lastUpdateCheck",                    m_lastUpdateCheck);
  reg.endGroup();               // settings.updates
  reg.endGroup();               // settings

  reg.beginGroup("defaults");
  reg.setValue("defaultTrackLanguage",               m_defaultTrackLanguage);
  reg.setValue("defaultChapterLanguage",             m_defaultChapterLanguage);
  reg.setValue("defaultChapterCountry",              m_defaultChapterCountry);
  reg.setValue("defaultSubtitleCharset",             m_defaultSubtitleCharset);
  reg.setValue("defaultAdditionalMergeOptions",      m_defaultAdditionalMergeOptions);
  reg.endGroup();               // defaults
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
  auto path = bfs::path{ to_utf8(exe) };
  if (path.is_absolute())
    return exe;

  auto installPath   = mtx::sys::get_installation_path();
  auto potentialExes = QList<bfs::path>{} << (installPath / path) << (installPath / ".." / path);

#if defined(SYS_WINDOWS)
  for (auto &potentialExe : potentialExes)
    potentialExe.replace_extension(bfs::path{"exe"});
#endif  // SYS_WINDOWS

  for (auto const &potentialExe : potentialExes)
    if (bfs::exists(potentialExe))
      return to_qs(bfs::canonical(potentialExe).string());

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

}}}
