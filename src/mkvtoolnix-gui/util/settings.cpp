#include "common/common_pch.h"

#include "common/fs_sys_helpers.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/util/settings.h"

#include <QSettings>

Settings Settings::s_settings;

Settings::Settings()
{
  load();
}

Settings &
Settings::get() {
  return s_settings;
}

void
Settings::load() {
  QSettings reg;

  reg.beginGroup("settings");
  m_mkvmergeExe               = reg.value("mkvmergeExe", "mkvmerge").toString();
  m_priority                  = static_cast<ProcessPriority>(reg.value("priority", static_cast<int>(NormalPriority)).toInt());
  m_lastOpenDir               = QDir{reg.value("lastOpenDir").toString()};
  m_lastOutputDir             = QDir{reg.value("lastOutputDir").toString()};
  m_lastConfigDir             = QDir{reg.value("lastConfigDir").toString()};

  m_scanForPlaylistsPolicy    = static_cast<ScanForPlaylistsPolicy>(reg.value("scanForPlaylistsPolicy", static_cast<int>(AskBeforeScanning)).toInt());
  m_minimumPlaylistDuration   = reg.value("minimumPlaylistDuration", 120).toUInt();

  m_setAudioDelayFromFileName = reg.value("setAudioDelayFromFileName", true).toBool();
  m_disableAVCompression      = reg.value("disableAVCompression",      false).toBool();
  m_autoSetFileTitle          = reg.value("autoSetFileTitle",          true).toBool();

  m_uniqueOutputFileNames     = reg.value("uniqueOutputFileNames",     true).toBool();
  m_outputFileNamePolicy      = static_cast<OutputFileNamePolicy>(reg.value("outputFileNamePolicy", static_cast<int>(ToPreviousDirectory)).toInt());
  m_fixedOutputDir            = QDir{reg.value("fixedOutputDir").toString()};
  reg.endGroup();

  reg.beginGroup("defaults");
  m_defaultTrackLanguage = reg.value("defaultTrackLanguage", Q("und")).toString();
  reg.endGroup();
}

QString
Settings::actualMkvmergeExe()
  const {
  return exeWithPath(m_mkvmergeExe);
}

void
Settings::save()
  const {
  QSettings reg;

  reg.beginGroup("settings");
  reg.setValue("mkvmergeExe",               m_mkvmergeExe);
  reg.setValue("priority",                  static_cast<int>(m_priority));
  reg.setValue("lastOpenDir",               m_lastOpenDir.path());
  reg.setValue("lastOutputDir",             m_lastOutputDir.path());
  reg.setValue("lastConfigDir",             m_lastConfigDir.path());

  reg.setValue("scanForPlaylistsPolicy",    static_cast<int>(m_scanForPlaylistsPolicy));
  reg.setValue("minimumPlaylistDuration",   m_minimumPlaylistDuration);

  reg.setValue("setAudioDelayFromFileName", m_setAudioDelayFromFileName);
  reg.setValue("autoSetFileTitle",          m_autoSetFileTitle);
  reg.setValue("disableAVCompression",      m_disableAVCompression);

  reg.setValue("outputFileNamePolicy",      static_cast<int>(m_outputFileNamePolicy));
  reg.setValue("fixedOutputDir",            m_fixedOutputDir.path());
  reg.setValue("uniqueOutputFileNames",     m_uniqueOutputFileNames);
  reg.endGroup();

  reg.beginGroup("defaults");
  reg.setValue("defaultTrackLanguage", m_defaultTrackLanguage);
  reg.endGroup();
}

QString
Settings::getPriorityAsString()
  const {
  return LowestPriority == m_priority ? Q("lowest")
       : LowPriority    == m_priority ? Q("lower")
       : NormalPriority == m_priority ? Q("normal")
       : HighPriority   == m_priority ? Q("higher")
       :                                Q("highest");
}

QString
Settings::exeWithPath(QString const &exe) {
#if defined(SYS_WINDOWS)
  auto path = bfs::path{ to_utf8(exe) };
  if (path.is_absolute())
    return exe;

  return to_qs((mtx::get_installation_path() / path).native());
#else  // defined(SYS_WINDOWS)
  return exe;
#endif // defined(SYS_WINDOWS)
}
