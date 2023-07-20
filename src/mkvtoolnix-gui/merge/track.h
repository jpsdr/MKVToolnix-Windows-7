#pragma once

#include "common/common_pch.h"

#include <QString>
#include <QVariant>

#include "common/audio_emphasis.h"
#include "common/bcp47.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/merge/enums.h"
#include "mkvtoolnix-gui/merge/mux_config.h"

namespace mtx::gui::Merge {

class SourceFile;
class MkvmergeOptionBuilder;

class Track;
using TrackPtr = std::shared_ptr<Track>;

class Track {
public:
  QVariantMap m_properties;

  SourceFile *m_file{};
  Track *m_appendedTo{};
  QList<Track *> m_appendedTracks;

  TrackType m_type{TrackType::Audio};
  int64_t m_id{-1};

  bool m_muxThis{true}, m_setAspectRatio{}, m_defaultTrackFlag{}, m_defaultTrackFlagWasSet{}, m_forcedTrackFlagWasSet{}, m_trackEnabledFlagWasSet{}, m_aacSbrWasDetected{}, m_nameWasPresent{}, m_fixBitstreamTimingInfo{}, m_reduceAudioToCore{};
  bool m_hearingImpairedFlag{}, m_hearingImpairedFlagWasSet{}, m_visualImpairedFlag{}, m_visualImpairedFlagWasSet{}, m_textDescriptionsFlag{}, m_textDescriptionsFlagWasSet{};
  bool m_originalFlag{}, m_originalFlagWasSet{}, m_commentaryFlag{}, m_commentaryFlagWasSet{};
  bool m_removeDialogNormalizationGain{};
  mtx::bcp47::language_c m_language;
  QString m_name, m_codec, m_tags, m_delay, m_stretchBy, m_defaultDuration, m_timestamps, m_aspectRatio, m_displayWidth, m_displayHeight, m_cropping, m_characterSet, m_additionalOptions;
  unsigned int m_forcedTrackFlag{}, m_trackEnabledFlag{}, m_stereoscopy{}, m_cues{}, m_aacIsSBR{};
  audio_emphasis_c::mode_e m_audioEmphasis{audio_emphasis_c::unspecified};
  TrackCompression m_compression{TrackCompression::Default};

  QString m_colorMatrixCoefficients, m_bitsPerColorChannel, m_chromaSubsampling, m_cbSubsampling, m_chromaSiting, m_colorRange, m_transferCharacteristics, m_colorPrimaries, m_maximumContentLight, m_maximumFrameLight;
  QString m_chromaticityCoordinates, m_whiteColorCoordinates, m_maximumLuminance, m_minimumLuminance;
  QString m_projectionType, m_projectionSpecificData, m_yawRotation, m_pitchRotation, m_rollRotation;

  int64_t m_size{};
  QString m_attachmentDescription;

  int m_colorIndex{};

public:
  explicit Track(SourceFile *file = nullptr, TrackType = TrackType::Audio);
  virtual ~Track();

  virtual bool isType(TrackType type) const;
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
  virtual bool canRemoveDialogNormalizationGain() const;
  virtual bool canSetAacToSbr() const;

  virtual void setDefaults(mtx::bcp47::language_c const &languageDerivedFromFileName);
  virtual void setDefaultsNonRegular();
  virtual QString extractAudioDelayFromFileName() const;

  virtual void saveSettings(Util::ConfigFile &settings) const;
  virtual void loadSettings(MuxConfig::Loader &l);
  virtual void fixAssociations(MuxConfig::Loader &l);

  virtual QString nameForType() const;

  virtual int mkvmergeTrackId() const;

  virtual std::string debugInfo() const;

  void buildMkvmergeOptions(MkvmergeOptionBuilder &opt) const;

protected:
  void setDefaultsBasics();
  void setDefaultsMuxThis();
  void setDefaultsForcedDisplayFlag();
  void setDefaultsDisplayDimensions();
  void setDefaultsLanguage(mtx::bcp47::language_c const &languageDerivedFromFileName);
  void setDefaultsColor();

public:
  static QString nameForType(TrackType type);
};

}

Q_DECLARE_METATYPE(mtx::gui::Merge::Track *)
