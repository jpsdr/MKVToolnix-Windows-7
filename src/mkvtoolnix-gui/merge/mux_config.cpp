#include "common/common_pch.h"

#include "common/at_scope_exit.h"
#include "common/logger.h"
#include "common/strings/editing.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/merge/attachment.h"
#include "mkvtoolnix-gui/merge/mux_config.h"
#include "mkvtoolnix-gui/merge/source_file.h"
#include "mkvtoolnix-gui/merge/track.h"
#include "mkvtoolnix-gui/util/file_identifier.h"
#include "mkvtoolnix-gui/util/settings.h"

#include <QDir>
#include <QFile>
#include <QStringList>
#include <QTemporaryFile>

namespace mtx { namespace gui { namespace Merge {

using namespace mtx::gui;

static void
addToMaps(SourceFile *oldFile,
          SourceFile *newFile,
          QHash<SourceFile *, SourceFile *> &fileMap,
          QHash<Track *, Track *> &trackMap) {
  Q_ASSERT(!!oldFile && !!newFile);

  fileMap[oldFile] = newFile;

  for (auto idx = 0, end = oldFile->m_tracks.size(); idx < end; ++idx)
    trackMap[ oldFile->m_tracks[idx].get() ] = newFile->m_tracks[idx].get();

  for (auto idx = 0, end = oldFile->m_additionalParts.size(); idx < end; ++idx)
    addToMaps(oldFile->m_additionalParts[idx].get(), newFile->m_additionalParts[idx].get(), fileMap, trackMap);

  for (auto idx = 0, end = oldFile->m_appendedFiles.size(); idx < end; ++idx)
    addToMaps(oldFile->m_appendedFiles[idx].get(), newFile->m_appendedFiles[idx].get(), fileMap, trackMap);
}

static void
fixMappings(SourceFile *oldFile,
            QHash<SourceFile *, SourceFile *> &fileMap,
            QHash<Track *, Track *> &trackMap) {
  auto newFile = fileMap[oldFile];

  Q_ASSERT(!!oldFile && !!newFile);

  if (oldFile->m_appendedTo) {
    newFile->m_appendedTo = fileMap[oldFile->m_appendedTo];
    Q_ASSERT(!!newFile->m_appendedTo);
  }

  for (auto idx = 0, end = oldFile->m_tracks.size(); idx < end; ++idx) {
    auto oldTrack = oldFile->m_tracks[idx].get();
    auto newTrack = trackMap[oldTrack];

    Q_ASSERT(!!oldTrack && !!newTrack);

    newTrack->m_file       = fileMap[oldTrack->m_file];
    newTrack->m_appendedTo = trackMap[oldTrack->m_appendedTo];

    Q_ASSERT((!!newTrack->m_file == !!oldTrack->m_file) && (!!newTrack->m_appendedTo == !!oldTrack->m_appendedTo));

    newTrack->m_appendedTracks.clear();
    for (auto const &oldAppendedTrack : oldTrack->m_appendedTracks) {
      auto newAppendedTrack = trackMap[oldAppendedTrack];
      Q_ASSERT(!!newAppendedTrack);
      newTrack->m_appendedTracks << newAppendedTrack;
    }
  }

  for (auto const &oldAppendedFile : oldFile->m_appendedFiles)
    fixMappings(oldAppendedFile.get(), fileMap, trackMap);
}

MuxConfig::MuxConfig(QString const &fileName)
  : m_configFileName{fileName}
  , m_splitMode{DoNotSplit}
  , m_splitMaxFiles{1}
  , m_linkFiles{false}
  , m_webmMode{false}
{
  auto &settings      = Util::Settings::get();
  m_additionalOptions = settings.m_defaultAdditionalMergeOptions;
}

MuxConfig::~MuxConfig() {
}

MuxConfig::MuxConfig(MuxConfig const &other)
{
  *this = other;
}

MuxConfig &
MuxConfig::operator =(MuxConfig const &other) {
  if (&other == this)
    return *this;

  m_configFileName                = other.m_configFileName;
  m_firstInputFileName            = other.m_firstInputFileName;
  m_title                         = other.m_title;
  m_destination                   = other.m_destination;
  m_destinationAuto               = other.m_destinationAuto;
  m_globalTags                    = other.m_globalTags;
  m_segmentInfo                   = other.m_segmentInfo;
  m_splitOptions                  = other.m_splitOptions;
  m_segmentUIDs                   = other.m_segmentUIDs;
  m_previousSegmentUID            = other.m_previousSegmentUID;
  m_nextSegmentUID                = other.m_nextSegmentUID;
  m_chapters                      = other.m_chapters;
  m_chapterLanguage               = other.m_chapterLanguage;
  m_chapterCharacterSet           = other.m_chapterCharacterSet;
  m_chapterCueNameFormat          = other.m_chapterCueNameFormat;
  m_additionalOptions             = other.m_additionalOptions;
  m_splitMode                     = other.m_splitMode;
  m_splitMaxFiles                 = other.m_splitMaxFiles;
  m_linkFiles                     = other.m_linkFiles;
  m_webmMode                      = other.m_webmMode;
  m_chapterGenerationMode         = other.m_chapterGenerationMode;
  m_chapterGenerationNameTemplate = other.m_chapterGenerationNameTemplate;
  m_chapterGenerationInterval     = other.m_chapterGenerationInterval;

  m_files.clear();
  m_tracks.clear();
  m_attachments.clear();

  for (auto const &attachment : other.m_attachments)
    m_attachments << std::make_shared<Attachment>(*attachment);

  auto fileMap  = QHash<SourceFile *, SourceFile *>{};
  auto trackMap = QHash<Track *, Track *>{};

  for (auto const &oldFile : other.m_files) {
    auto newFile = std::make_shared<SourceFile>(*oldFile);
    m_files << newFile;

    addToMaps(oldFile.get(), newFile.get(), fileMap, trackMap);
  }

  for (auto const &oldFile : other.m_files)
    fixMappings(oldFile.get(), fileMap, trackMap);

  for (auto const &oldTrack : other.m_tracks) {
    auto newTrack = trackMap[oldTrack];
    Q_ASSERT(!!newTrack);
    m_tracks << newTrack;
  }

  return *this;
}

void
MuxConfig::loadProperties(Util::ConfigFile &settings,
                          QVariantMap &properties) {
  properties.clear();

  settings.beginGroup("properties");
  for (auto &key : settings.childKeys())
    properties[key] = settings.value(key);
  settings.endGroup();
}

void
MuxConfig::load(QString const &fileName) {
  auto fileNameToOpen = fileName.isEmpty() ? m_configFileName : fileName;

  if (fileNameToOpen.isEmpty())
    throw InvalidSettingsX{};

  auto settings = Util::ConfigFile::open(fileNameToOpen);
  if (!settings)
    throw InvalidSettingsX{};

  load(*settings);

  m_configFileName = fileNameToOpen;
}

QString
MuxConfig::determineFirstInputFileName(QList<SourceFilePtr> const &files) {
  if (files.isEmpty())
    return QString{};

  if (!Util::Settings::get().m_autoDestinationOnlyForVideoFiles)
    return files[0]->m_fileName;

  auto itr = brng::find_if(files, [](auto const &file) { return file->hasVideoTrack(); });
  return itr != files.end() ? (*itr)->m_fileName : QString{};
}

void
MuxConfig::load(Util::ConfigFile &settings) {
  reset();

  // Check supported config file version
  if (settings.childGroups().contains(App::settingsBaseGroupName())) {
    settings.beginGroup(App::settingsBaseGroupName());
    if (   (settings.value("version", std::numeric_limits<int>::max()).toInt() > MTXCFG_VERSION)
        || (settings.value("type").toString() != settingsType()))
      throw InvalidSettingsX{};
    settings.endGroup();

  } else if (settings.value("version", std::numeric_limits<int>::max()).toInt() > MTXCFG_VERSION)
    // Config files written until 8.0.0 didn't use that group.
    throw InvalidSettingsX{};

  settings.beginGroup("input");

  QHash<qulonglong, SourceFile *> objectIDToSourceFile;
  QHash<qulonglong, Track *> objectIDToTrack;
  Loader l{settings, objectIDToSourceFile, objectIDToTrack};

  loadSettingsGroup<SourceFile>("files",       m_files,       l);
  loadSettingsGroup<Attachment>("attachments", m_attachments, l);

  settings.beginGroup("files");
  auto idx = 0u;
  for (auto &file : m_files) {
    settings.beginGroup(QString::number(idx++));
    file->fixAssociations(l);
    settings.endGroup();

    file->m_fileName = QDir::toNativeSeparators(file->m_fileName);
  }
  settings.endGroup();

  // Load track list.
  for (auto trackID : settings.value("trackOrder").toList()) {
    auto track = objectIDToTrack.value(trackID.toLongLong());

    if (!track)
      throw InvalidSettingsX{};

    // Compatibility with older settings: there attached files were
    // stored together with the other tracks.
    if (track->isAttachment())
      continue;

    m_tracks << track;
  }

  m_firstInputFileName = settings.value("firstInputFileName", determineFirstInputFileName(m_files)).toString();
  m_firstInputFileName = QDir::toNativeSeparators(m_firstInputFileName);

  settings.endGroup();

  // Load global settings
  settings.beginGroup("global");
  m_title                         = settings.value("title").toString();
  m_destination                   = QDir::toNativeSeparators(settings.value("destination").toString());
  m_destinationAuto               = QDir::toNativeSeparators(settings.value("destinationAuto").toString());
  m_globalTags                    = QDir::toNativeSeparators(settings.value("globalTags").toString());
  m_segmentInfo                   = QDir::toNativeSeparators(settings.value("segmentInfo").toString());
  m_splitOptions                  = settings.value("splitOptions").toString();
  m_segmentUIDs                   = settings.value("segmentUIDs").toString();
  m_previousSegmentUID            = settings.value("previousSegmentUID").toString();
  m_nextSegmentUID                = settings.value("nextSegmentUID").toString();
  m_chapters                      = QDir::toNativeSeparators(settings.value("chapters").toString());
  m_chapterLanguage               = settings.value("chapterLanguage").toString();
  m_chapterCharacterSet           = settings.value("chapterCharacterSet").toString();
  m_chapterCueNameFormat          = settings.value("chapterCueNameFormat").toString();
  m_additionalOptions             = settings.value("additionalOptions").toString();
  m_splitMode                     = static_cast<SplitMode>(settings.value("splitMode").toInt());
  m_splitMaxFiles                 = std::max(settings.value("splitMaxFiles").toInt(), 1);
  m_linkFiles                     = settings.value("linkFiles").toBool();
  m_webmMode                      = settings.value("webmMode").toBool();
  m_chapterGenerationMode         = static_cast<ChapterGenerationMode>(settings.value("chapterGenerationMode").toInt());
  m_chapterGenerationNameTemplate = settings.value("chapterGenerationNameTemplate").toString();
  m_chapterGenerationInterval     = settings.value("chapterGenerationInterval").toString();
  settings.endGroup();
}

void
MuxConfig::saveProperties(Util::ConfigFile &settings,
                          QVariantMap const &properties) {
  QStringList keys{ properties.keys() };
  keys.sort();
  settings.beginGroup("properties");
  for (auto &key : keys)
    settings.setValue(key, properties.value(key));
  settings.endGroup();
}

void
MuxConfig::save(Util::ConfigFile &settings)
  const {
  settings.beginGroup(App::settingsBaseGroupName());
  settings.setValue("version", MTXCFG_VERSION);
  settings.setValue("type",    settingsType());
  settings.endGroup();

  settings.beginGroup("input");
  saveSettingsGroup("files",       m_files,       settings);
  saveSettingsGroup("attachments", m_attachments, settings);

  settings.setValue("trackOrder",         std::accumulate(m_tracks.begin(), m_tracks.end(), QList<QVariant>{}, [](QList<QVariant> &accu, Track *track) { accu << QVariant{reinterpret_cast<qulonglong>(track)}; return accu; }));
  settings.setValue("firstInputFileName", m_firstInputFileName);
  settings.endGroup();

  settings.beginGroup("global");
  settings.setValue("title",                         m_title);
  settings.setValue("destination",                   m_destination);
  settings.setValue("destinationAuto",               m_destinationAuto);
  settings.setValue("globalTags",                    m_globalTags);
  settings.setValue("segmentInfo",                   m_segmentInfo);
  settings.setValue("splitOptions",                  m_splitOptions);
  settings.setValue("segmentUIDs",                   m_segmentUIDs);
  settings.setValue("previousSegmentUID",            m_previousSegmentUID);
  settings.setValue("nextSegmentUID",                m_nextSegmentUID);
  settings.setValue("chapters",                      m_chapters);
  settings.setValue("chapterLanguage",               m_chapterLanguage);
  settings.setValue("chapterCharacterSet",           m_chapterCharacterSet);
  settings.setValue("chapterCueNameFormat",          m_chapterCueNameFormat);
  settings.setValue("additionalOptions",             m_additionalOptions);
  settings.setValue("splitMode",                     m_splitMode);
  settings.setValue("splitMaxFiles",                 m_splitMaxFiles);
  settings.setValue("linkFiles",                     m_linkFiles);
  settings.setValue("webmMode",                      m_webmMode);
  settings.setValue("chapterGenerationMode",         static_cast<int>(m_chapterGenerationMode));
  settings.setValue("chapterGenerationNameTemplate", m_chapterGenerationNameTemplate);
  settings.setValue("chapterGenerationInterval",     m_chapterGenerationInterval);
  settings.endGroup();
}

void
MuxConfig::save(QString const &fileName) {
  if (!fileName.isEmpty())
    m_configFileName = fileName;
  if (m_configFileName.isEmpty())
    return;

  QFile::remove(m_configFileName);
  auto settings = Util::ConfigFile::create(m_configFileName);
  save(*settings);
  settings->save();
}

QString
MuxConfig::toString()
  const {
  auto tempFileName = QString{};

  at_scope_exit_c cleaner([&tempFileName]() { QFile{tempFileName}.remove(); });

  {
    QTemporaryFile tempFile{QDir::temp().filePath(Q("MKVToolNix-GUI-MuxConfig-XXXXXX"))};
    tempFile.setAutoRemove(false);
    if (!tempFile.open())
      return QString{};
    tempFileName = tempFile.fileName();
  }

  auto settings = Util::ConfigFile::create(tempFileName);
  save(*settings);
  settings->save();

  QFile file{tempFileName};
  if (file.open(QIODevice::ReadOnly))
    return QString::fromUtf8(file.readAll());

  return QString{};
}

void
MuxConfig::reset() {
  *this = MuxConfig{};
}

MuxConfigPtr
MuxConfig::loadSettings(QString const &fileName) {
  auto config = std::make_shared<MuxConfig>(fileName);
  config->load();

  return config;
}

QHash<SourceFile *, unsigned int>
MuxConfig::buildFileNumbers()
  const {
  auto fileNumbers = QHash<SourceFile *, unsigned int>{};
  auto number      = 0u;
  for (auto const &file : m_files) {
    fileNumbers[file.get()] = number++;
    for (auto const &appendedFile : file->m_appendedFiles)
      fileNumbers[appendedFile.get()] = number++;
  }

  return fileNumbers;
}

QStringList
MuxConfig::buildTrackOrder(QHash<SourceFile *, unsigned int> const &fileNumbers)
  const {
  auto trackOrder = QStringList{};
  for (auto const &track : m_tracks)
    if (   track->m_muxThis
        && (!track->m_appendedTo || track->m_appendedTo->m_muxThis)
        && (track->isAudio() || track->isVideo() || track->isSubtitles() || track->isButtons()))
      trackOrder << Q("%1:%2").arg(fileNumbers.value(track->m_file)).arg(track->m_id);

  if (trackOrder.size() > 1)
    return QStringList{} << Q("--track-order") << trackOrder.join(Q(","));
  return QStringList{};
}

QStringList
MuxConfig::buildAppendToMapping(QHash<SourceFile *, unsigned int> const &fileNumbers)
  const {
  auto appendToMapping = QStringList{};
  for (auto const &destinationTrack : m_tracks) {
    auto currentDestinationFileNumber = fileNumbers.value(destinationTrack->m_file);
    auto currentDestinationTrackId    = destinationTrack->m_id;

    for (auto const &sourceTrack : destinationTrack->m_appendedTracks)
      if (sourceTrack->m_muxThis && (sourceTrack->isAudio() || sourceTrack->isVideo() || sourceTrack->isSubtitles() || sourceTrack->isButtons())) {
        appendToMapping << Q("%1:%2:%3:%4").arg(fileNumbers.value(sourceTrack->m_file)).arg(sourceTrack->m_id).arg(currentDestinationFileNumber).arg(currentDestinationTrackId);

        currentDestinationFileNumber = fileNumbers.value(sourceTrack->m_file);
        currentDestinationTrackId    = sourceTrack->m_id;
      }
  }

  return appendToMapping.isEmpty() ? QStringList{} : QStringList{} << Q("--append-to") << appendToMapping.join(Q(","));
}

QStringList
MuxConfig::buildMkvmergeOptions()
  const {
  auto options = QStringList{};

  auto &settings = Util::Settings::get();
  auto locale    = settings.localeToUse();

  if (!locale.isEmpty())
    options << Q("--ui-language") << locale;

  if (Util::Settings::NormalPriority != settings.m_priority)
    options << Q("--priority") << settings.priorityAsString();

  options << Q("--output") << m_destination;

  if (m_webmMode)
    options << Q("--webm");

  auto probeRangePercentage = 0.0;

  for (auto const &file : m_files) {
    file->buildMkvmergeOptions(options);
    probeRangePercentage = std::max(probeRangePercentage, file->m_probeRangePercentage);
  }

  for (auto const &attachment : m_attachments)
    attachment->buildMkvmergeOptions(options);

  if (DoNotSplit != m_splitMode) {
    auto mode = SplitAfterSize       == m_splitMode ? Q("size:")
              : SplitAfterDuration   == m_splitMode ? Q("duration:")
              : SplitAfterTimestamps == m_splitMode ? Q("timestamps:")
              : SplitByParts         == m_splitMode ? Q("parts:")
              : SplitByPartsFrames   == m_splitMode ? Q("parts-frames:")
              : SplitByFrames        == m_splitMode ? Q("frames:")
              : SplitAfterChapters   == m_splitMode ? Q("chapters:")
              :                                       Q("PROGRAM EROR");
    options << Q("--split") << (mode + m_splitOptions);

    if (m_splitMaxFiles >= 2)
      options << Q("--split-max-files") << QString::number(m_splitMaxFiles);
    if (m_linkFiles)
      options << Q("--link");
  }

  auto add = [&options](auto const &arg, auto const &value, boost::logic::tribool predicate = boost::indeterminate) {
    if (boost::logic::indeterminate(predicate))
      predicate = !value.isEmpty();
    if (predicate)
      options << arg << value;
  };

  add(Q("--title"),            m_title, !m_title.isEmpty() || hasSourceFileWithTitle());
  add(Q("--segment-uid"),      m_segmentUIDs);
  add(Q("--link-to-previous"), m_previousSegmentUID);
  add(Q("--link-to-next"),     m_nextSegmentUID);
  add(Q("--segmentinfo"),      m_segmentInfo);

  if (!m_chapters.isEmpty()) {
    add(Q("--chapter-language"),        m_chapterLanguage);
    add(Q("--chapter-charset"),         m_chapterCharacterSet);
    add(Q("--cue-chapter-name-format"), m_chapterCueNameFormat);
    options << Q("--chapters") << m_chapters;
  }

  if (ChapterGenerationMode::None != m_chapterGenerationMode) {
    add(Q("--chapter-language"),                m_chapterLanguage);
    add(Q("--generate-chapters-name-template"), m_chapterGenerationNameTemplate);

    options << Q("--generate-chapters");

    if (ChapterGenerationMode::WhenAppending == m_chapterGenerationMode)
      options << Q("when-appending");

    else
      options << Q("interval:%1").arg(m_chapterGenerationInterval);
  }

  add(Q("--global-tags"), m_globalTags);

  auto additionalOptions = Q(strip_copy(to_utf8(m_additionalOptions)));
  if (!additionalOptions.isEmpty())
    options += additionalOptions.split(QRegExp{" +"});

  auto fileNumbers  = buildFileNumbers();
  options          += buildTrackOrder(fileNumbers);
  options          += buildAppendToMapping(fileNumbers);

  Util::FileIdentifier::addProbeRangePercentageArg(options, probeRangePercentage);

  return options;
}

bool
MuxConfig::hasSourceFileWithTitle()
  const {
  for (auto const &sourceFile : m_files)
    if (!sourceFile->m_properties.value("title").toString().isEmpty())
      return true;

  return false;
}

void
MuxConfig::debugDumpFileList()
  const {
  auto num = m_files.count();
  log_it(boost::format("// Dumping file list with %1% entries\n") % num);

  for (auto idx = 0; idx < num; ++idx) {
    auto const &file = *m_files[idx];
    log_it(boost::format("%1%/%2% %3%\n") % idx % num % to_utf8(QFileInfo{file.m_fileName}.fileName()));

    for (auto addIdx = 0, addNum = file.m_additionalParts.count(); addIdx < addNum; ++addIdx)
      log_it(boost::format("  = %1%/%2% %3%\n") % addIdx % addNum % to_utf8(QFileInfo{file.m_additionalParts[addIdx]->m_fileName}.fileName()));

    for (auto appIdx = 0, appNum = file.m_appendedFiles.count(); appIdx < appNum; ++appIdx)
      log_it(boost::format("  + %1%/%2% %3%\n") % appIdx % appNum % to_utf8(QFileInfo{file.m_appendedFiles[appIdx]->m_fileName}.fileName()));
  }
}

void
MuxConfig::debugDumpTrackList()
  const {
  debugDumpSpecificTrackList(m_tracks);
}

void
MuxConfig::debugDumpSpecificTrackList(QList<Track *> const &tracks) {
  auto num = tracks.count();
  log_it(boost::format("// Dumping track list with %1% entries\n") % num);

  for (auto idx = 0; idx < num; ++idx) {
    auto const &track = *tracks[idx];
    log_it(boost::format("%1%/%2% %3% %4% from %5%\n") % idx % num % track.nameForType() % track.m_codec % to_utf8(QFileInfo{track.m_file->m_fileName}.fileName()));

    for (auto appIdx = 0, appNum = track.m_appendedTracks.count(); appIdx < appNum; ++appIdx) {
      auto const &appTrack = *track.m_appendedTracks[appIdx];
      log_it(boost::format("  %1%/%2% %3% %4% from %5%\n") % appIdx % appNum % appTrack.nameForType() % appTrack.m_codec % to_utf8(QFileInfo{appTrack.m_file->m_fileName}.fileName()));
    }
  }
}

QString
MuxConfig::settingsType() {
  return Q("MuxConfig");
}

}}}
