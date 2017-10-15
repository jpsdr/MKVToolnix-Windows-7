#pragma once

#include "common/common_pch.h"

#include <QString>
#include <QVariant>

#include "common/qt.h"
#include "mkvtoolnix-gui/merge/mux_config.h"

namespace mtx { namespace gui { namespace Merge {

class SourceFile;
class MkvmergeOptionBuilder;

class Track;
using TrackPtr = std::shared_ptr<Track>;

class Track {
public:
  enum Type {
    Audio = 0,
    Video,
    Subtitles,
    Buttons,
    Chapters,
    GlobalTags,
    Tags,
    Attachment,
    TypeMin = Audio,
    TypeMax = Attachment
  };

  enum Compression {
    CompDefault = 0,
    CompNone,
    CompZlib,
    CompMin = CompDefault,
    CompMax = CompZlib
  };

  QVariantMap m_properties;

  SourceFile *m_file{};
  Track *m_appendedTo{};
  QList<Track *> m_appendedTracks;

  Type m_type{Audio};
  int64_t m_id{-1};

  bool m_muxThis{true}, m_setAspectRatio{true}, m_defaultTrackFlagWasSet{}, m_defaultTrackFlagWasPresent{}, m_forcedTrackFlagWasSet{}, m_aacSbrWasDetected{}, m_nameWasPresent{}, m_fixBitstreamTimingInfo{}, m_reduceAudioToCore{};
  QString m_name, m_codec, m_language, m_tags, m_delay, m_stretchBy, m_defaultDuration, m_timestamps, m_aspectRatio, m_displayWidth, m_displayHeight, m_cropping, m_characterSet, m_additionalOptions;
  unsigned int m_defaultTrackFlag{}, m_forcedTrackFlag{}, m_stereoscopy{}, m_naluSizeLength{}, m_cues{}, m_aacIsSBR{};
  Compression m_compression{CompDefault};
  boost::optional<bool> m_effectiveDefaultTrackFlag;

  int64_t m_size{};
  QString m_attachmentDescription;

public:
  explicit Track(SourceFile *file = nullptr, Type = Audio);
  virtual ~Track();

  virtual bool isType(Type type) const;
  virtual bool isAudio() const;
  virtual bool isVideo() const;
  virtual bool isSubtitles() const;
  virtual bool isButtons() const;
  virtual bool isChapters() const;
  virtual bool isGlobalTags() const;
  virtual bool isTags() const;
  virtual bool isAttachment() const;
  virtual bool isRegular() const;
  virtual bool isAppended() const;

  virtual bool isPropertySet(QString const &property) const;
  virtual bool canChangeSubCharset() const;
  virtual bool canReduceToAudioCore() const;
  virtual bool canSetAacToSbr() const;

  virtual void setDefaults();
  virtual QString extractAudioDelayFromFileName() const;

  virtual void saveSettings(Util::ConfigFile &settings) const;
  virtual void loadSettings(MuxConfig::Loader &l);
  virtual void fixAssociations(MuxConfig::Loader &l);

  virtual QString nameForType() const;

  virtual std::string debugInfo() const;

  void buildMkvmergeOptions(MkvmergeOptionBuilder &opt) const;
};

}}}

Q_DECLARE_METATYPE(mtx::gui::Merge::Track *)
