#include "common/common_pch.h"

#include <QDebug>
#include <QRegularExpression>

#include "common/iso639.h"
#include "common/list_utils.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/merge/mkvmerge_option_builder.h"
#include "mkvtoolnix-gui/merge/mux_config.h"
#include "mkvtoolnix-gui/merge/source_file.h"
#include "mkvtoolnix-gui/util/settings.h"

#include <QDir>

namespace mtx::gui::Merge {

namespace {

template<typename T>
void
fixAssociationsFor(char const *group,
                   QList<std::shared_ptr<T> > &container,
                   MuxConfig::Loader &l) {
  l.settings.beginGroup(group);

  int idx = 0;
  for (auto &entry : container) {
    l.settings.beginGroup(QString::number(idx));
    entry->fixAssociations(l);
    l.settings.endGroup();
    ++idx;
  }

  l.settings.endGroup();
}

}

SourceFile::SourceFile(QString const &fileName)
  : m_fileName{QDir::toNativeSeparators(fileName)}
{

}

SourceFile::SourceFile(SourceFile const &other)
{
  *this = other;
}

SourceFile::~SourceFile() {
}

SourceFile &
SourceFile::operator =(SourceFile const &other) {
  if (this == &other)
    return *this;

  m_properties                = other.m_properties;
  m_programMap                = other.m_programMap;
  m_fileName                  = other.m_fileName;
  m_type                      = other.m_type;
  m_playlistFiles             = other.m_playlistFiles;
  m_appended                  = other.m_appended;
  m_additionalPart            = other.m_additionalPart;
  m_isPlaylist                = other.m_isPlaylist;
  m_dontScanForOtherPlaylists = other.m_dontScanForOtherPlaylists;
  m_appendedTo                = other.m_appendedTo;
  m_playlistDuration          = other.m_playlistDuration;
  m_playlistSize              = other.m_playlistSize;
  m_playlistChapters          = other.m_playlistChapters;
  m_probeRangePercentage      = other.m_probeRangePercentage;

  m_tracks.clear();
  m_attachedFiles.clear();
  m_additionalParts.clear();
  m_appendedFiles.clear();

  for (auto const &track : other.m_tracks) {
    m_tracks << std::make_shared<Track>(*track);
    m_tracks.back()->m_file = this;
  }

  for (auto const &attachedFile : other.m_attachedFiles) {
    m_attachedFiles << std::make_shared<Track>(*attachedFile);
    m_attachedFiles.back()->m_file = this;
  }

  for (auto const &file : other.m_additionalParts)
    m_additionalParts << std::make_shared<SourceFile>(*file);

  for (auto const &file : other.m_appendedFiles)
    m_appendedFiles << std::make_shared<SourceFile>(*file);

  return *this;
}

bool
SourceFile::isValid()
  const {
  return !container().isEmpty() || isAdditionalPart();
}


bool
SourceFile::isRegular()
  const {
  return !m_appended && !m_additionalPart;
}

bool
SourceFile::isAppended()
  const {
  return m_appended;
}

bool
SourceFile::isAdditionalPart()
  const {
  return m_additionalPart;
}

bool
SourceFile::isPlaylist()
  const {
  return m_isPlaylist;
}

bool
SourceFile::hasRegularTrack()
  const {
  return m_tracks.end() != std::find_if(m_tracks.begin(), m_tracks.end(), [](TrackPtr const &track) { return track->isRegular(); });
}

bool
SourceFile::hasAudioTrack()
  const {
  return m_tracks.end() != std::find_if(m_tracks.begin(), m_tracks.end(), [](TrackPtr const &track) { return track->isAudio(); });
}

bool
SourceFile::hasSubtitlesTrack()
  const {
  return m_tracks.end() != std::find_if(m_tracks.begin(), m_tracks.end(), [](TrackPtr const &track) { return track->isSubtitles(); });
}

bool
SourceFile::hasVideoTrack()
  const {
  return m_tracks.end() != std::find_if(m_tracks.begin(), m_tracks.end(), [](TrackPtr const &track) { return track->isVideo(); });
}

QString
SourceFile::container()
  const {
  return Q(file_type_t::get_name(m_type).get_translated());
}

bool
SourceFile::isTextSubtitleContainer()
  const {
  return mtx::included_in(m_type, mtx::file_type_e::srt, mtx::file_type_e::ssa);
}

void
SourceFile::saveSettings(Util::ConfigFile &settings)
  const {
  MuxConfig::saveProperties(settings, m_properties);

  saveSettingsGroup("tracks",          m_tracks,          settings);
  saveSettingsGroup("attachedFiles",   m_attachedFiles,   settings);
  saveSettingsGroup("additionalParts", m_additionalParts, settings);
  saveSettingsGroup("appendedFiles",   m_appendedFiles,   settings);

  settings.setValue("objectID",        reinterpret_cast<qulonglong>(this));
  settings.setValue("fileName",        m_fileName);
  settings.setValue("type",            static_cast<unsigned int>(m_type));
  settings.setValue("appended",        m_appended);
  settings.setValue("additionalPart",  m_additionalPart);
  settings.setValue("appendedTo",      reinterpret_cast<qulonglong>(m_appendedTo));

  auto playlistFiles = QStringList{};
  for (auto const &playlistFile : m_playlistFiles)
    playlistFiles << playlistFile.filePath();

  settings.setValue("isPlaylist",           m_isPlaylist);
  settings.setValue("playlistFiles",        playlistFiles);
  settings.setValue("playlistDuration",     static_cast<qulonglong>(m_playlistDuration));
  settings.setValue("playlistSize",         static_cast<qulonglong>(m_playlistSize));
  settings.setValue("playlistChapters",     static_cast<qulonglong>(m_playlistChapters));

  settings.setValue("probeRangePercentage", m_probeRangePercentage);
}

void
SourceFile::loadSettings(MuxConfig::Loader &l) {
  auto objectID = l.settings.value("objectID").toULongLong();
  if ((0 >= objectID) || l.objectIDToSourceFile.contains(objectID))
    throw InvalidSettingsX{};

  l.objectIDToSourceFile[objectID] = this;
  m_fileName                       = l.settings.value("fileName").toString();
  m_type                           = static_cast<file_type_e>(l.settings.value("type").toInt());
  m_appended                       = l.settings.value("appended").toBool();
  m_additionalPart                 = l.settings.value("additionalPart").toBool();
  m_appendedTo                     = reinterpret_cast<SourceFile *>(l.settings.value("appendedTo").toULongLong());

  m_isPlaylist                     = l.settings.value("isPlaylist").toBool();
  auto playlistFiles               = l.settings.value("playlistFiles").toStringList();
  m_playlistDuration               = l.settings.value("playlistDuration").toULongLong();
  m_playlistSize                   = l.settings.value("playlistSize").toULongLong();
  m_playlistChapters               = l.settings.value("playlistChapters").toULongLong();

  m_probeRangePercentage           = l.settings.value("probeRangePercentage", 0.3).toDouble();

  m_playlistFiles.clear();
  for (auto const &playlistFile : playlistFiles)
    m_playlistFiles << QFileInfo{playlistFile};

  if ((mtx::file_type_e::is_unknown > m_type) || (mtx::file_type_e::max < m_type))
    throw InvalidSettingsX{};

  MuxConfig::loadProperties(l.settings, m_properties);

  loadSettingsGroup<Track>     ("tracks",          m_tracks,          l);
  loadSettingsGroup<Track>     ("attachedFiles",   m_attachedFiles,   l);
  loadSettingsGroup<SourceFile>("additionalParts", m_additionalParts, l);
  loadSettingsGroup<SourceFile>("appendedFiles",   m_appendedFiles,   l);

  setupProgramMapFromProperties();

  // Compatibility with older settings: there attached files were
  // stored together with the other tracks.
  int idx = 0;
  while (idx < m_tracks.count()) {
    auto &track = m_tracks[idx];
    if (track->isAttachment()) {
      m_attachedFiles << track;
      m_tracks.removeAt(idx);

    } else
      ++idx;
  }
}

void
SourceFile::fixAssociations(MuxConfig::Loader &l) {
  if (isRegular() || isAdditionalPart())
    m_appendedTo = nullptr;

  else {
    auto appendedToID = reinterpret_cast<qulonglong>(m_appendedTo);
    if ((0 >= appendedToID) || !l.objectIDToSourceFile.contains(appendedToID))
      throw InvalidSettingsX{};
    m_appendedTo = l.objectIDToSourceFile.value(appendedToID);
  }

  for (auto &track : m_tracks)
    track->m_file = this;

  for (auto &attachedFile : m_attachedFiles)
    attachedFile->m_file = this;

  fixAssociationsFor("tracks",          m_tracks,          l);
  fixAssociationsFor("attachedFiles",   m_attachedFiles,   l);
  fixAssociationsFor("additionalParts", m_additionalParts, l);
  fixAssociationsFor("appendedFiles",   m_appendedFiles,   l);
}

Track *
SourceFile::findNthOrLastTrackOfType(TrackType type,
                                     int nth)
  const {
  auto nthFound   = -1;
  auto lastOfType = static_cast<Track *>(nullptr);

  for (auto const &track : m_tracks)
    if (track->m_type == type) {
      ++nthFound;
      if (nthFound == nth)
        return track.get();

      lastOfType = track.get();
    }

  return lastOfType;
}

void
SourceFile::buildMkvmergeOptions(Util::CommandLineOptions &options)
  const {
  Q_ASSERT(!isAdditionalPart());

  auto opt = MkvmergeOptionBuilder{};

  for (auto const &track : m_tracks)
    track->buildMkvmergeOptions(opt);

  for (auto const &attachedFile : m_attachedFiles)
    attachedFile->buildMkvmergeOptions(opt);

  auto buildTrackIdArg = [&options,&opt](TrackType type, QString const &enabled, QString const &disabled) {
    if (!enabled.isEmpty() && !opt.enabledTrackIds[type].isEmpty() && (static_cast<unsigned int>(opt.enabledTrackIds[type].size()) < opt.numTracksOfType[type]))
      options << enabled << opt.enabledTrackIds[type].join(u","_s);

    else if (opt.enabledTrackIds[type].isEmpty() && opt.numTracksOfType[type])
      options << disabled;
  };

  buildTrackIdArg(TrackType::Audio,      u"--audio-tracks"_s,    u"--no-audio"_s);
  buildTrackIdArg(TrackType::Video,      u"--video-tracks"_s,    u"--no-video"_s);
  buildTrackIdArg(TrackType::Subtitles,  u"--subtitle-tracks"_s, u"--no-subtitles"_s);
  buildTrackIdArg(TrackType::Buttons,    u"--button-tracks"_s,   u"--no-buttons"_s);
  buildTrackIdArg(TrackType::Attachment, u"--attachments"_s,     u"--no-attachments"_s);
  buildTrackIdArg(TrackType::Tags,       u"--track-tags"_s,      u"--no-track-tags"_s);
  buildTrackIdArg(TrackType::GlobalTags, u""_s,                  u"--no-global-tags"_s);
  buildTrackIdArg(TrackType::Chapters,   u""_s,                  u"--no-chapters"_s);

  options << opt.options;
  if (m_appendedTo)
    options << u"+"_s;
  options << u"("_s << Util::CommandLineOption::fileName(m_fileName);
  for (auto const &additionalPart : m_additionalParts)
    options << Util::CommandLineOption::fileName(additionalPart->m_fileName);
  options << u")"_s;

  for (auto const &appendedFile : m_appendedFiles)
    appendedFile->buildMkvmergeOptions(options);
}

void
SourceFile::setDefaults() {
  auto languageDerivedFromFileName = deriveLanguageFromFileName();

  for (auto const &track : m_tracks)
    track->setDefaults(languageDerivedFromFileName);

  for (auto const &attachedFile : m_attachedFiles)
    attachedFile->setDefaults(languageDerivedFromFileName);

  for (auto const &appendedFile : m_appendedFiles)
    appendedFile->setDefaults();
}

void
SourceFile::setupProgramMapFromProperties() {
  m_programMap.clear();

  if (!m_properties.contains(u"programs"_s))
    return;

  for (auto const &program : m_properties[u"programs"_s].toList()) {
    auto programProps = program.toMap();
    if (programProps.contains(u"program_number"_s))
      m_programMap.insert(programProps[u"program_number"_s].toUInt(), { programProps[u"service_provider"_s].toString(), programProps[u"service_name"_s].toString() });
  }
}

mtx::bcp47::language_c
SourceFile::deriveLanguageFromFileName() {
  auto &cfg     = Util::Settings::get();
  auto fileName = QFileInfo{m_fileName}.fileName().toLower();
  auto matches  = QRegularExpression{u"s\\d+e\\d{2,}(.+)"_s}.match(fileName);

  if (matches.hasMatch()) {
    fileName = matches.captured(1);
    qDebug() << "found season/episode code; only using postfix:" << fileName;
  }

  QStringList escapedChars;

  // First try to detect full BCP 47 language tags if they contain at
  // least two components & don't start with x-. As BCP 47 tags
  // contain '-' characters, don't split by them.
  for (auto c : cfg.m_boundaryCharsForDerivingTrackLanguagesFromFileNames)
    if (c != QChar{'-'})
      escapedChars << QRegularExpression::escape(c);

  QRegularExpression bcp47Re{u"^[^x][a-z]+-"_s, QRegularExpression::CaseInsensitiveOption};

  if (!escapedChars.isEmpty()) {
    auto splitRE     = QRegularExpression{u"(?:%1)+"_s.arg(escapedChars.join(u"|"_s))};
    auto allCaptures = fileName.split(splitRE);

    for (auto captureItr = allCaptures.rbegin(), captureEnd = allCaptures.rend(); captureItr != captureEnd; ++captureItr) {
      auto &capture = *captureItr;

      if (capture.isEmpty())
        continue;

      qDebug() << "language derivation match (BCP 47):" << capture;

      if (!capture.contains(bcp47Re))
        continue;

      auto tag = mtx::bcp47::language_c::parse(to_utf8(capture));
      if (tag.is_valid()) {
        qDebug() << "derived BCP 47 language tag";
        return tag;
      }
    }
  }

  // No full BCP 47 language tag found. Now look for languages only
  // with the full set of boundary characters.
  escapedChars.clear();

  for (auto c : cfg.m_boundaryCharsForDerivingTrackLanguagesFromFileNames)
    escapedChars << QRegularExpression::escape(c);

  auto splitRE     = QRegularExpression{u"(?:%1)+"_s.arg(escapedChars.join(u"|"_s))};
  auto allCaptures = fileName.split(splitRE);

  if (allCaptures.size() < 2) {
    qDebug() << "language could not be derived: no match found";
    return {};
  }

  allCaptures.removeLast();

  qDebug() << "potential matches:" << allCaptures;

  QRegularExpression languageNameCleaner{u" *[\\(,].*"_s}, splitter{u" *; *"_s};

  for (auto captureItr = allCaptures.rbegin(), captureEnd = allCaptures.rend(); captureItr != captureEnd; ++captureItr) {
    auto &capture = *captureItr;

    if (capture.isEmpty())
      continue;

    qDebug() << "language derivation match:" << capture;

    auto languageOpt = mtx::iso639::look_up(to_utf8(capture));
    if (languageOpt) {
      if (!cfg.m_recognizedTrackLanguagesInFileNames.contains(Q(languageOpt->alpha_3_code)))
        continue;

      qDebug() << "derived language via code lookup:" << Q(languageOpt->alpha_3_code);

      return mtx::bcp47::language_c::parse(to_utf8(Q(languageOpt->alpha_3_code)));
    }

    for (auto const &languageElt : mtx::iso639::g_languages) {
      if (!cfg.m_recognizedTrackLanguagesInFileNames.contains(Q(languageElt.alpha_3_code)))
        continue;

      for (auto name : Q(languageElt.english_name).split(splitter)) {
        name.remove(languageNameCleaner);
        if (name.toLower() == capture) {
          qDebug() << "derived language via name:" << Q(languageElt.alpha_3_code);

          return mtx::bcp47::language_c::parse(to_utf8(Q(languageElt.alpha_3_code)));
        }
      }
    }
  }

  qDebug() << "language could not be derived: match found but no mapping to language:" << matches.capturedTexts();

  return {};
}

}
