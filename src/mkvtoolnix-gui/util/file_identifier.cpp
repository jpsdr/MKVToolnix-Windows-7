#include "common/common_pch.h"

#include <QMessageBox>
#include <QRegularExpression>
#include <QStringList>

#include "common/qt.h"
#include "common/strings/editing.h"
#include "mkvtoolnix-gui/util/file_identifier.h"
#include "mkvtoolnix-gui/util/message_box.h"
#include "mkvtoolnix-gui/util/process.h"
#include "mkvtoolnix-gui/util/settings.h"

namespace mtx { namespace gui { namespace Util {

using namespace mtx::gui;

FileIdentifier::FileIdentifier(QWidget *parent,
                               QString const &fileName)
  : m_parent(parent)
  , m_exitCode(0)
  , m_fileName(fileName)
{
}

FileIdentifier::~FileIdentifier() {
}

bool
FileIdentifier::identify() {
  if (m_fileName.isEmpty())
    return false;

  auto &cfg = Settings::get();

  QStringList args;
  args << "--output-charset" << "utf-8" << "--identify-for-gui" << m_fileName;

  if (cfg.m_defaultAdditionalMergeOptions.contains(Q("keep_last_chapter_in_mpls")))
    args << "--engage" << "keep_last_chapter_in_mpls";

  auto process  = Process::execute(cfg.actualMkvmergeExe(), args);
  auto exitCode = process->process().exitCode();

  if (process->hasError()) {
    Util::MessageBox::critical(m_parent)->title(QY("Error executing mkvmerge")).text(QY("The mkvmerge executable was not found.").arg(exitCode)).exec();
    return false;
  }

  m_output = process->output();

  if (0 == exitCode)
    return parseOutput();

  if (3 == exitCode) {
    auto pos       = m_output.isEmpty() ? -1            : m_output[0].indexOf("container:");
    auto container = -1 == pos          ? QY("unknown") : m_output[0].mid(pos + 11);

    Util::MessageBox::critical(m_parent)->title(QY("Unsupported file format")).text(QY("The file is an unsupported container format (%1).").arg(container)).exec();

    return false;
  }

  Util::MessageBox::critical(m_parent)->title(QY("Unrecognized file format")).text(QY("The file was not recognized as a supported format (exit code: %1).").arg(exitCode)).exec();

  return false;
}

QString const &
FileIdentifier::fileName()
  const {
  return m_fileName;
}

void
FileIdentifier::setFileName(QString const &fileName) {
  m_fileName = fileName;
}

int
FileIdentifier::exitCode()
  const {
  return m_exitCode;
}

QStringList const &
FileIdentifier::output()
  const {
  return m_output;
}

Merge::SourceFilePtr const &
FileIdentifier::file()
  const {
  return m_file;
}

bool
FileIdentifier::parseOutput() {
  m_file = std::make_shared<Merge::SourceFile>(m_fileName);

  for (auto &line : m_output) {
    if (line.startsWith("File"))
      parseContainerLine(line);

    else if (line.startsWith("Track"))
      parseTrackLine(line);

    else if (line.startsWith("Attachment"))
      parseAttachmentLine(line);

    else if (line.startsWith("Chapters"))
      parseChaptersLine(line);

    else if (line.startsWith("Global tags"))
      parseGlobalTagsLine(line);

    else if (line.startsWith("Tags"))
      parseTagsLine(line);

    else if (line.startsWith("Track"))
      parseTrackLine(line);
  }

  return m_file->isValid();
}

// Attachment ID 1: type "cue", size 1844 bytes, description "dummy", file name "cuewithtags2.cue"
// The »description« and »file name« parts are optional.
void
FileIdentifier::parseAttachmentLine(QString const &line) {
  static QRegularExpression s_re{"^Attachment ID (\\d+): type \"(.*)\", size (\\d+) bytes(?:, description \"(.*)\")?(?:, file name \"(.*)\")"};
  auto matches = s_re.match(line);

  if (!matches.hasMatch())
    return;

  auto track                     = std::make_shared<Merge::Track>(m_file.get(), Merge::Track::Attachment);
  track->m_properties            = parseProperties(line);
  track->m_id                    = matches.captured(1).toLongLong();
  track->m_codec                 = matches.captured(2);
  track->m_size                  = matches.captured(3).toLongLong();
  track->m_attachmentDescription = matches.captured(4);
  track->m_name                  = matches.captured(5);

  m_file->m_tracks << track;
}

// Chapters: 27 entries
void
FileIdentifier::parseChaptersLine(QString const &line) {
  QRegExp re{"^Chapters: (\\d+) entries$"};

  if (-1 == re.indexIn(line))
    return;

  auto track    = std::make_shared<Merge::Track>(m_file.get(), Merge::Track::Chapters);
  track->m_size = re.cap(1).toLongLong();

  m_file->m_tracks << track;
}

// File 'complex.mkv': container: Matroska [duration:106752000000 segment_uid:00000000000000000000000000000000]
void
FileIdentifier::parseContainerLine(QString const &line) {
  QRegExp re{"^File\\s.*container:\\s+([^\\[]+)"};

  if (-1 == re.indexIn(line))
    return;

  m_file->m_properties       = parseProperties(line);
  m_file->m_type             = static_cast<file_type_e>(m_file->m_properties["container_type"].toInt());
  m_file->m_isPlaylist       = m_file->m_properties["playlist"] == "1";
  m_file->m_playlistDuration = m_file->m_properties["playlist_duration"].toULongLong();
  m_file->m_playlistSize     = m_file->m_properties["playlist_size"].toULongLong();
  m_file->m_playlistChapters = m_file->m_properties["playlist_chapters"].toULongLong();

  if (m_file->m_isPlaylist && !m_file->m_properties["playlist_file"].isEmpty())
    for (auto const &fileName : m_file->m_properties["playlist_file"].split("\t"))
      m_file->m_playlistFiles << QFileInfo{fileName};

  if (m_file->m_properties["other_file"].isEmpty())
    return;

  for (auto &fileName : m_file->m_properties["other_file"].split("\t")) {
    auto additionalPart              = std::make_shared<Merge::SourceFile>(fileName);
    additionalPart->m_additionalPart = true;
    additionalPart->m_appendedTo     = m_file.get();
    m_file->m_additionalParts       << additionalPart;
  }
}

// Global tags: 3 entries
void
FileIdentifier::parseGlobalTagsLine(QString const &line) {
  QRegExp re{"^Global tags: (\\d+) entries$"};

  if (-1 == re.indexIn(line))
    return;

  auto track    = std::make_shared<Merge::Track>(m_file.get(), Merge::Track::GlobalTags);
  track->m_size = re.cap(1).toLongLong();

  m_file->m_tracks << track;
}

// Tags for track ID 1: 2 entries
void
FileIdentifier::parseTagsLine(QString const &line) {
  QRegExp re{"^Tags for track ID (\\d+): (\\d+) entries$"};

  if (-1 == re.indexIn(line))
    return;

  auto track    = std::make_shared<Merge::Track>(m_file.get(), Merge::Track::Tags);
  track->m_id   = re.cap(1).toLongLong();
  track->m_size = re.cap(2).toLongLong();

  m_file->m_tracks << track;
}

// Track ID 0: video (V_MS/VFW/FOURCC, DIV3) [number:1 ...]
// Track ID 7: audio (A_PCM/INT/LIT) [number:8 uid:289972206 codec_id:A_PCM/INT/LIT codec_private_length:0 language:und default_track:0 forced_track:0 enabled_track:1 default_duration:31250000 audio_sampling_frequency:48000 audio_channels:2]
// Track ID 8: subtitles (S_TEXT/UTF8) [number:9 ...]
void
FileIdentifier::parseTrackLine(QString const &line) {
  QRegExp re{"Track\\s+ID\\s+(\\d+):\\s+(audio|video|subtitles|buttons)\\s+\\(([^\\)]+)\\)", Qt::CaseInsensitive};
  if (-1 == re.indexIn(line))
    return;

  auto type           = re.cap(2) == "audio"     ? Merge::Track::Audio
                      : re.cap(2) == "video"     ? Merge::Track::Video
                      : re.cap(2) == "subtitles" ? Merge::Track::Subtitles
                      :                            Merge::Track::Buttons;
  auto track          = std::make_shared<Merge::Track>(m_file.get(), type);
  track->m_id         = re.cap(1).toLongLong();
  track->m_codec      = re.cap(3);
  track->m_properties = parseProperties(line);

  m_file->m_tracks << track;

  track->setDefaults();
}

QHash<QString, QString>
FileIdentifier::parseProperties(QString const &line)
  const {
  QHash<QString, QString> properties;

  QRegularExpression reLine{".*\\[(.*?)\\]\\s*$"};
  QRegularExpression rePair{"(.+):(.+)"};

  auto match = reLine.match(line);
  if (!match.hasMatch())
    return properties;

  for (auto &pair : match.captured(1).split(QRegExp{"\\s+"}, QString::SkipEmptyParts)) {
    match = rePair.match(pair);
    if (!match.hasMatch())
      continue;

    auto key = to_qs(unescape(to_utf8(match.captured(1))));
    if (!properties[key].isEmpty())
      properties[key] += Q("\t");
    properties[key] += to_qs(unescape(to_utf8(match.captured(2))));
  }

  return properties;
}

}}}
