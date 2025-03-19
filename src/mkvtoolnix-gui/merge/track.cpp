#include "common/common_pch.h"

#include <QRegularExpression>
#include <QVariant>
#include <QDebug>

#include "common/id_info.h"
#include "common/iso639.h"
#include "common/list_utils.h"
#include "common/qt6_compat/meta_type.h"
#include "common/strings/editing.h"
#include "common/strings/formatting.h"
#include "mkvtoolnix-gui/merge/enums.h"
#include "mkvtoolnix-gui/merge/mkvmerge_option_builder.h"
#include "mkvtoolnix-gui/merge/mux_config.h"
#include "mkvtoolnix-gui/merge/source_file.h"
#include "mkvtoolnix-gui/merge/track.h"
#include "mkvtoolnix-gui/util/command_line_options.h"
#include "mkvtoolnix-gui/util/config_file.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "qregularexpression.h"

namespace mtx::gui::Merge {

namespace {

std::vector<std::pair<std::string, std::string>> trackSettingsNameReplacements{
  { "bitsPerColourChannel"s,     "bitsPerColorChannel"s     },
  { "colourMatrixCoefficients"s, "colorMatrixCoefficients"s },
  { "colourRange"s,              "colorRange"s              },
  { "colourPrimaries"s,          "colorPrimaries"s          },
  { "whiteColourCoordinates"s,   "whiteColorCoordinates"s   },
};

} // anonymous namespace

using namespace mtx::gui;

Track::Track(SourceFile *file,
             TrackType type)
  : m_file{file}
  , m_type{type}
{
}

Track::~Track() {
}

bool
Track::isType(TrackType type)
  const {
  return type == m_type;
}

bool
Track::isAudio()
  const {
  return isType(TrackType::Audio);
}

bool
Track::isVideo()
  const {
  return isType(TrackType::Video);
}

bool
Track::isSubtitles()
  const {
  return isType(TrackType::Subtitles);
}

bool
Track::isButtons()
  const {
  return isType(TrackType::Buttons);
}

bool
Track::isChapters()
  const {
  return isType(TrackType::Chapters);
}

bool
Track::isGlobalTags()
  const {
  return isType(TrackType::GlobalTags);
}

bool
Track::isTags()
  const {
  return isType(TrackType::Tags);
}

bool
Track::isAttachment()
  const {
  return isType(TrackType::Attachment);
}

bool
Track::isAppended()
  const {
  return !m_file ? false : m_file->m_appended;
}

bool
Track::isRegular()
  const {
  return isAudio() || isVideo() || isSubtitles() || isButtons();
}

bool
Track::isForcedSubtitles()
  const {
  if (!isSubtitles())
    return false;

  if (m_forcedTrackFlag == 1)
    return true;

  auto &settings = Util::Settings::get();
  QRegularExpression re{settings.m_regexForRecognizingForcedSubtitleNames, QRegularExpression::CaseInsensitiveOption};

  if (re.isValid() && m_name.contains(re))
    return true;

  return false;
}

bool
Track::isPropertySet(QString const &property)
  const {
  if (!m_properties.contains(property))
    return false;

  auto var = m_properties.value(property);

  return var.isNull()                                         ? false
       : !var.isValid()                                       ? false
       : mtxCanConvertVariantTo(var, QMetaType::QVariantList) ? !var.toList().isEmpty()
       : mtxCanConvertVariantTo(var, QMetaType::QString)      ? !var.toString().isEmpty()
       :                                                        true;
}

void
Track::setDefaultsNonRegular() {
  auto &settings = Util::Settings::get();

  if (!settings.m_enableMuxingTracksByTheseTypes.contains(m_type))
    m_muxThis = false;
}

void
Track::setDefaultsLanguage(mtx::bcp47::language_c const &languageDerivedFromFileName) {
  auto &settings        = Util::Settings::get();
  auto languageProperty = m_properties.value(Q(mtx::id::language_ietf)).toString();

  if (languageProperty.isEmpty())
    languageProperty = m_properties.value(Q(mtx::id::language)).toString();

  auto language = mtx::bcp47::language_c::parse(to_utf8(languageProperty));

  if (languageDerivedFromFileName.is_valid()) {
    auto policy = isAudio()     ? settings.m_deriveAudioTrackLanguageFromFileNamePolicy
                : isVideo()     ? settings.m_deriveVideoTrackLanguageFromFileNamePolicy
                : isSubtitles() ? settings.m_deriveSubtitleTrackLanguageFromFileNamePolicy
                :                 Util::Settings::DeriveLanguageFromFileNamePolicy::Never;

    if (   ((policy != Util::Settings::DeriveLanguageFromFileNamePolicy::Never)                  && !language.is_valid())
        || ((policy == Util::Settings::DeriveLanguageFromFileNamePolicy::IfAbsentOrUndetermined) && (language.get_language() == "und"s)))
      language = languageDerivedFromFileName;
  }

  if (   !language.is_valid()
      ||  (Util::Settings::SetDefaultLanguagePolicy::Always                 == settings.m_whenToSetDefaultLanguage)
      || ((Util::Settings::SetDefaultLanguagePolicy::IfAbsentOrUndetermined == settings.m_whenToSetDefaultLanguage) && (language.get_language() == "und"s)))
    language = isAudio()     ? settings.m_defaultAudioTrackLanguage
             : isVideo()     ? settings.m_defaultVideoTrackLanguage
             : isSubtitles() ? settings.m_defaultSubtitleTrackLanguage
             :                 mtx::bcp47::language_c{};

  if (language.is_valid())
    m_language = language;
}

void
Track::setDefaultsDisplayDimensions() {
  QRegularExpression re_displayDimensions{"^(\\d+)x(\\d+)$"};
  auto matches = re_displayDimensions.match(m_properties.value(Q(mtx::id::display_dimensions)).toString());
  if (!matches.hasMatch())
    return;

  m_displayWidth  = matches.captured(1);
  m_displayHeight = matches.captured(2);
}

void
Track::setDefaultsMuxThis() {
  auto &settings = Util::Settings::get();

  if (settings.m_enableMuxingForcedSubtitleTracks && isForcedSubtitles()) {
    m_muxThis = true;
    return;
  }

  if (!settings.m_enableMuxingTracksByTheseTypes.contains(m_type)) {
    m_muxThis = false;
    return;
  }

  if (   !settings.m_enableMuxingTracksByLanguage
      || !mtx::included_in(m_type, TrackType::Video, TrackType::Audio, TrackType::Subtitles)
      || ((TrackType::Video     == m_type) && settings.m_enableMuxingAllVideoTracks)
      || ((TrackType::Audio     == m_type) && settings.m_enableMuxingAllAudioTracks)
      || ((TrackType::Subtitles == m_type) && settings.m_enableMuxingAllSubtitleTracks)) {
    m_muxThis = true;
    return;
  }

  auto language = !m_language.is_valid() ? mtx::bcp47::language_c::parse("und"s) : m_language;
  m_muxThis     = settings.m_enableMuxingTracksByTheseLanguages.contains(Q(language.get_iso639_alpha_3_code()));
}

void
Track::setDefaultsCommentaryFlag() {
  auto &settings = Util::Settings::get();

  QRegularExpression re{settings.m_regexForDerivingCommentaryFlagFromFileNames, QRegularExpression::CaseInsensitiveOption};

  if (!re.isValid() || !settings.m_deriveCommentaryFlagFromFileNames || isAppended() || (!isAudio() && !isSubtitles()))
    return;

  qDebug() << "testing for commentary, name is" << m_name;

  if (m_file->m_fileName.contains(re) || (settings.m_deriveFlagsFromTrackNames && m_name.contains(re)))
    m_commentaryFlag = true;
}

void
Track::setDefaultsHearingImpairedFlag() {
  auto &settings = Util::Settings::get();

  QRegularExpression re{settings.m_regexForDerivingHearingImpairedFlagFromFileNames, QRegularExpression::CaseInsensitiveOption};

  if (!re.isValid() || !settings.m_deriveHearingImpairedFlagFromFileNames || isAppended() || (!isAudio() && !isSubtitles()))
    return;

  if (m_file->m_fileName.contains(re) || (settings.m_deriveFlagsFromTrackNames && m_name.contains(re)))
    m_hearingImpairedFlag = true;
}

void
Track::setDefaultsForcedDisplayFlag() {
  auto &settings = Util::Settings::get();

  QRegularExpression re{settings.m_regexForDerivingSubtitlesForcedFlagFromFileNames, QRegularExpression::CaseInsensitiveOption};

  if (!re.isValid() || !settings.m_deriveSubtitlesForcedFlagFromFileNames || isAppended() || !isSubtitles())
    return;

  if (m_file->m_fileName.contains(re) || (settings.m_deriveFlagsFromTrackNames && m_name.contains(re)))
    m_forcedTrackFlag = 1;
}

void
Track::setDefaultsOriginalLanguageFlag() {
  auto &settings = Util::Settings::get();

  if (!settings.m_defaultSetOriginalLanguageFlagLanguage.is_valid() || isAppended() || (!isAudio() && !isSubtitles()))
    return;

  if (m_language.matches(settings.m_defaultSetOriginalLanguageFlagLanguage))
    m_originalFlag = 1;
}

void
Track::setDefaultsBasics() {
  auto &settings = Util::Settings::get();

  m_forcedTrackFlag               = m_properties.value(Q(mtx::id::forced_track)).toBool() ? 1 : 0;
  m_forcedTrackFlagWasSet         = m_forcedTrackFlag == 1;
  m_trackEnabledFlag              = m_properties.value(Q(mtx::id::enabled_track), true).toBool() ? 1 : 0;
  m_trackEnabledFlagWasSet        = m_trackEnabledFlag == 1;
  m_hearingImpairedFlag           = m_properties.value(Q(mtx::id::flag_hearing_impaired)).toBool();
  m_hearingImpairedFlagWasSet     = m_hearingImpairedFlag;
  m_visualImpairedFlag            = m_properties.value(Q(mtx::id::flag_visual_impaired)).toBool();
  m_visualImpairedFlagWasSet      = m_visualImpairedFlag;
  m_textDescriptionsFlag          = m_properties.value(Q(mtx::id::flag_text_descriptions)).toBool();
  m_textDescriptionsFlagWasSet    = m_textDescriptionsFlag;
  m_originalFlag                  = m_properties.value(Q(mtx::id::flag_original)).toBool();
  m_originalFlagWasSet            = m_originalFlag;
  m_commentaryFlag                = m_properties.value(Q(mtx::id::flag_commentary)).toBool();
  m_commentaryFlagWasSet          = m_commentaryFlag;
  m_defaultTrackFlag              = m_properties.contains(Q(mtx::id::default_track)) ? m_properties.value(Q(mtx::id::default_track)).toBool() : true;
  m_defaultTrackFlagWasSet        = m_defaultTrackFlag;
  m_name                          = m_properties.value(Q(mtx::id::track_name)).toString();
  m_nameWasPresent                = !m_name.isEmpty();
  m_cropping                      = m_properties.value(Q(mtx::id::cropping)).toString();
  m_aacSbrWasDetected             = m_properties.value(Q(mtx::id::aac_is_sbr)).toString().contains(QRegularExpression{"1|true"});
  m_stereoscopy                   = m_properties.contains(Q(mtx::id::stereo_mode)) ? m_properties.value(Q(mtx::id::stereo_mode)).toUInt() + 1 : 0;
  auto encoding                   = m_properties.value(Q(Q(mtx::id::encoding))).toString();
  m_characterSet                  = !encoding.isEmpty()   ? encoding
                                  : canChangeSubCharset() ? settings.m_defaultSubtitleCharset
                                  :                         Q("");
  m_delay                         = isAudio() && settings.m_setAudioDelayFromFileName ? extractAudioDelayFromFileName() : QString{};
  m_removeDialogNormalizationGain = canRemoveDialogNormalizationGain() && settings.m_mergeEnableDialogNormGainRemoval;
  auto emphasis                   = m_properties.value(Q(mtx::id::audio_emphasis), static_cast<int>(audio_emphasis_c::unspecified)).toInt();
  m_audioEmphasis                 = audio_emphasis_c::valid_index(emphasis) ? static_cast<audio_emphasis_c::mode_e>(emphasis) : audio_emphasis_c::unspecified;
}

void
Track::setDefaultsColor() {
  auto toDoubleIfSet = [this](QString const &key) -> QString {
    if (!m_properties.contains(key) || m_properties[key].toString().isEmpty())
      return {};
    return Q(mtx::string::normalize_fmt_double_output(m_properties[key].toDouble()));
  };

  m_bitsPerColorChannel     = m_properties.value(Q(mtx::id::color_bits_per_channel)).toString();
  m_colorMatrixCoefficients = m_properties.value(Q(mtx::id::color_matrix_coefficients)).toString();
  m_colorPrimaries          = m_properties.value(Q(mtx::id::color_primaries)).toString();
  m_colorRange              = m_properties.value(Q(mtx::id::color_range)).toString();
  m_transferCharacteristics = m_properties.value(Q(mtx::id::color_transfer_characteristics)).toString();
  m_maximumContentLight     = m_properties.value(Q(mtx::id::max_content_light)).toString();
  m_maximumFrameLight       = m_properties.value(Q(mtx::id::max_frame_light)).toString();
  m_maximumLuminance        = toDoubleIfSet(Q(mtx::id::max_luminance));
  m_minimumLuminance        = toDoubleIfSet(Q(mtx::id::min_luminance));
  m_pitchRotation           = toDoubleIfSet(Q(mtx::id::projection_pose_pitch));
  m_rollRotation            = toDoubleIfSet(Q(mtx::id::projection_pose_roll));
  m_yawRotation             = toDoubleIfSet(Q(mtx::id::projection_pose_yaw));
  m_projectionSpecificData  = m_properties.value(Q(mtx::id::projection_private)).toString();
  m_projectionType          = m_properties.value(Q(mtx::id::projection_type)).toString();
  m_cbSubsampling           = m_properties.value(Q(mtx::id::cb_subsample)).toString();
  m_chromaSiting            = m_properties.value(Q(mtx::id::chroma_siting)).toString();
  m_chromaSubsampling       = m_properties.value(Q(mtx::id::chroma_subsample)).toString();
  m_whiteColorCoordinates   = m_properties.value(Q(mtx::id::white_color_coordinates)).toString();
  m_chromaticityCoordinates = m_properties.value(Q(mtx::id::chromaticity_coordinates)).toString();
}

void
Track::setDefaults(mtx::bcp47::language_c const &languageDerivedFromFileName) {
  if (!isRegular()) {
    setDefaultsNonRegular();
    return;
  }

  setDefaultsBasics();
  setDefaultsLanguage(languageDerivedFromFileName);
  setDefaultsForcedDisplayFlag();
  setDefaultsCommentaryFlag();
  setDefaultsHearingImpairedFlag();
  setDefaultsOriginalLanguageFlag();
  setDefaultsMuxThis();
  setDefaultsDisplayDimensions();
  setDefaultsColor();
}

QString
Track::extractAudioDelayFromFileName()
  const {
  auto matches = QRegularExpression{"delay\\s+(-?\\d+)", QRegularExpression::CaseInsensitiveOption}.match(m_file->m_fileName);
  if (matches.hasMatch())
    return matches.captured(1);
  return "";
}

void
Track::saveSettings(Util::ConfigFile &settings)
  const {
  for (auto const &pair : trackSettingsNameReplacements)
    settings.remove(Q(pair.first));

  MuxConfig::saveProperties(settings, m_properties);

  QStringList appendedTracks;
  for (auto &track : m_appendedTracks)
    appendedTracks << QString::number(reinterpret_cast<qulonglong>(track));

  settings.setValue("objectID",                      reinterpret_cast<qulonglong>(this));
  settings.setValue("appendedTo",                    reinterpret_cast<qulonglong>(m_appendedTo));
  settings.setValue("appendedTracks",                appendedTracks);
  settings.setValue("type",                          static_cast<int>(m_type));
  settings.setValue("id",                            static_cast<qulonglong>(m_id));
  settings.setValue("muxThis",                       m_muxThis);
  settings.setValue("setAspectRatio",                m_setAspectRatio);
  settings.setValue("defaultTrackFlagWasSet",        m_defaultTrackFlagWasSet);
  settings.setValue("forcedTrackFlagWasSet",         m_forcedTrackFlagWasSet);
  settings.setValue("trackEnabledFlagWasSet",        m_trackEnabledFlagWasSet);
  settings.setValue("aacSbrWasDetected",             m_aacSbrWasDetected);
  settings.setValue("nameWasPresent",                m_nameWasPresent);
  settings.setValue("fixBitstreamTimingInfo",        m_fixBitstreamTimingInfo);
  settings.setValue("name",                          m_name);
  settings.setValue("codec",                         m_codec);
  settings.setValue("language",                      Q(m_language.format()));
  settings.setValue("tags",                          m_tags);
  settings.setValue("delay",                         m_delay);
  settings.setValue("stretchBy",                     m_stretchBy);
  settings.setValue("defaultDuration",               m_defaultDuration);
  settings.setValue("timestamps",                    m_timestamps);
  settings.setValue("aspectRatio",                   m_aspectRatio);
  settings.setValue("displayWidth",                  m_displayWidth);
  settings.setValue("displayHeight",                 m_displayHeight);
  settings.setValue("cropping",                      m_cropping);
  settings.setValue("characterSet",                  m_characterSet);
  settings.setValue("additionalOptions",             m_additionalOptions);
  settings.setValue("defaultTrackFlag",              m_defaultTrackFlag);
  settings.setValue("forcedTrackFlag",               m_forcedTrackFlag);
  settings.setValue("trackEnabledFlag",              m_trackEnabledFlag);
  settings.setValue("stereoscopy",                   m_stereoscopy);
  settings.setValue("cues",                          m_cues);
  settings.setValue("aacIsSBR",                      m_aacIsSBR);
  settings.setValue("reduceAudioToCore",             m_reduceAudioToCore);
  settings.setValue("removeDialogNormalizationGain", m_removeDialogNormalizationGain);
  settings.setValue("audioEmphasis",                 static_cast<int>(m_audioEmphasis));
  settings.setValue("compression",                   static_cast<int>(m_compression));
  settings.setValue("size",                          static_cast<qulonglong>(m_size));
  settings.setValue("attachmentDescription",         m_attachmentDescription);
  settings.setValue("hearingImpairedFlag",           m_hearingImpairedFlag);
  settings.setValue("hearingImpairedFlagWasSet",     m_hearingImpairedFlagWasSet);
  settings.setValue("visualImpairedFlag",            m_visualImpairedFlag);
  settings.setValue("visualImpairedFlagWasSet",      m_visualImpairedFlagWasSet);
  settings.setValue("textDescriptionsFlag",          m_textDescriptionsFlag);
  settings.setValue("textDescriptionsFlagWasSet",    m_textDescriptionsFlagWasSet);
  settings.setValue("originalFlag",                  m_originalFlag);
  settings.setValue("originalFlagWasSet",            m_originalFlagWasSet);
  settings.setValue("commentaryFlag",                m_commentaryFlag);
  settings.setValue("commentaryFlagWasSet",          m_commentaryFlagWasSet);

  settings.setValue("colorMatrixCoefficients",       m_colorMatrixCoefficients);
  settings.setValue("bitsPerColorChannel",           m_bitsPerColorChannel);
  settings.setValue("chromaSubsampling",             m_chromaSubsampling);
  settings.setValue("cbSubsampling",                 m_cbSubsampling);
  settings.setValue("chromaSiting",                  m_chromaSiting);
  settings.setValue("colorRange",                    m_colorRange);
  settings.setValue("transferCharacteristics",       m_transferCharacteristics);
  settings.setValue("colorPrimaries",                m_colorPrimaries);
  settings.setValue("maximumContentLight",           m_maximumContentLight);
  settings.setValue("maximumFrameLight",             m_maximumFrameLight);

  settings.setValue("chromaticityCoordinates",       m_chromaticityCoordinates);
  settings.setValue("whiteColorCoordinates",         m_whiteColorCoordinates);
  settings.setValue("maximumLuminance",              m_maximumLuminance);
  settings.setValue("minimumLuminance",              m_minimumLuminance);

  settings.setValue("projectionType",                m_projectionType);
  settings.setValue("projectionSpecificData",        m_projectionSpecificData);
  settings.setValue("yawRotation",                   m_yawRotation);
  settings.setValue("pitchRotation",                 m_pitchRotation);
  settings.setValue("rollRotation",                  m_rollRotation);
}

void
Track::loadSettings(MuxConfig::Loader &l) {
  MuxConfig::loadProperties(l.settings, m_properties);

  auto objectID = l.settings.value("objectID").toULongLong();
  if ((0 >= objectID) || l.objectIDToTrack.contains(objectID))
    throw InvalidSettingsX{};

  for (auto const &pair : trackSettingsNameReplacements)
    if (l.settings.childKeys().contains(Q(pair.first)))
      l.settings.setValue(Q(pair.second), l.settings.value(Q(pair.first)));

  l.objectIDToTrack[objectID]     = this;
  m_type                          = static_cast<TrackType>(l.settings.value("type").toInt());
  m_id                            = l.settings.value("id").toULongLong();
  m_muxThis                       = l.settings.value("muxThis").toBool();
  m_setAspectRatio                = l.settings.value("setAspectRatio").toBool();
  m_defaultTrackFlagWasSet        = l.settings.value("defaultTrackFlagWasSet").toBool();
  m_forcedTrackFlagWasSet         = l.settings.value("forcedTrackFlagWasSet").toBool();
  m_trackEnabledFlagWasSet        = l.settings.value("trackEnabledFlagWasSet").toBool();
  m_aacSbrWasDetected             = l.settings.value("aacSbrWasDetected").toBool();
  m_name                          = l.settings.value("name").toString();
  m_nameWasPresent                = l.settings.value("nameWasPresent").toBool();
  m_fixBitstreamTimingInfo        = l.settings.value("fixBitstreamTimingInfo").toBool();
  m_codec                         = l.settings.value("codec").toString();
  m_language                      = mtx::bcp47::language_c::parse(to_utf8(l.settings.value("language").toString()));
  m_tags                          = l.settings.value("tags").toString();
  m_delay                         = l.settings.value("delay").toString();
  m_stretchBy                     = l.settings.value("stretchBy").toString();
  m_defaultDuration               = l.settings.value("defaultDuration").toString();
  m_timestamps                    = l.settings.value("timestamps", l.settings.value("timecodes").toString()).toString();
  m_aspectRatio                   = l.settings.value("aspectRatio").toString();
  m_displayWidth                  = l.settings.value("displayWidth").toString();
  m_displayHeight                 = l.settings.value("displayHeight").toString();
  m_cropping                      = l.settings.value("cropping").toString();
  m_characterSet                  = l.settings.value("characterSet").toString();
  m_additionalOptions             = l.settings.value("additionalOptions").toString();
  m_defaultTrackFlag              = l.settings.value("defaultTrackFlag").toBool();
  m_forcedTrackFlag               = l.settings.value("forcedTrackFlag").toInt();
  m_trackEnabledFlag              = l.settings.value("trackEnabledFlag").toInt();
  m_stereoscopy                   = l.settings.value("stereoscopy").toInt();
  m_cues                          = l.settings.value("cues").toInt();
  m_aacIsSBR                      = l.settings.value("aacIsSBR").toInt();
  m_reduceAudioToCore             = l.settings.value("reduceAudioToCore").toBool();
  m_removeDialogNormalizationGain = l.settings.value("removeDialogNormalizationGain").toBool();
  auto emphasis                   = l.settings.value("audioEmphasis", static_cast<int>(audio_emphasis_c::unspecified)).toInt();
  m_audioEmphasis                 = audio_emphasis_c::valid_index(emphasis) ? static_cast<audio_emphasis_c::mode_e>(emphasis) : audio_emphasis_c::unspecified;
  m_compression                   = static_cast<TrackCompression>(l.settings.value("compression").toInt());
  m_size                          = l.settings.value("size").toULongLong();
  m_attachmentDescription         = l.settings.value("attachmentDescription").toString();
  m_hearingImpairedFlag           = l.settings.value("hearingImpairedFlag").toBool();
  m_hearingImpairedFlagWasSet     = l.settings.value("hearingImpairedFlagWasSet").toBool();
  m_visualImpairedFlag            = l.settings.value("visualImpairedFlag").toBool();
  m_visualImpairedFlagWasSet      = l.settings.value("visualImpairedFlagWasSet").toBool();
  m_textDescriptionsFlag          = l.settings.value("textDescriptionsFlag").toBool();
  m_textDescriptionsFlagWasSet    = l.settings.value("textDescriptionsFlagWasSet").toBool();
  m_originalFlag                  = l.settings.value("originalFlag").toBool();
  m_originalFlagWasSet            = l.settings.value("originalFlagWasSet").toBool();
  m_commentaryFlag                = l.settings.value("commentaryFlag").toBool();
  m_commentaryFlagWasSet          = l.settings.value("commentaryFlagWasSet").toBool();

  m_colorMatrixCoefficients       = l.settings.value("colorMatrixCoefficients").toString();
  m_bitsPerColorChannel           = l.settings.value("bitsPerColorChannel").toString();
  m_chromaSubsampling             = l.settings.value("chromaSubsampling").toString();
  m_cbSubsampling                 = l.settings.value("cbSubsampling").toString();
  m_chromaSiting                  = l.settings.value("chromaSiting").toString();
  m_colorRange                    = l.settings.value("colorRange").toString();
  m_transferCharacteristics       = l.settings.value("transferCharacteristics").toString();
  m_colorPrimaries                = l.settings.value("colorPrimaries").toString();
  m_maximumContentLight           = l.settings.value("maximumContentLight").toString();
  m_maximumFrameLight             = l.settings.value("maximumFrameLight").toString();

  m_chromaticityCoordinates       = l.settings.value("chromaticityCoordinates").toString();
  m_whiteColorCoordinates         = l.settings.value("whiteColorCoordinates").toString();
  m_maximumLuminance              = l.settings.value("maximumLuminance").toString();
  m_minimumLuminance              = l.settings.value("minimumLuminance").toString();

  m_projectionType                = l.settings.value("projectionType").toString();
  m_projectionSpecificData        = l.settings.value("projectionSpecificData").toString();
  m_yawRotation                   = l.settings.value("yawRotation").toString();
  m_pitchRotation                 = l.settings.value("pitchRotation").toString();
  m_rollRotation                  = l.settings.value("rollRotation").toString();

  if (   (TrackType::Min        > m_type)        || (TrackType::Max        < m_type)
      || (TrackCompression::Min > m_compression) || (TrackCompression::Max < m_compression))
    throw InvalidSettingsX{};
}

void
Track::fixAssociations(MuxConfig::Loader &l) {
  if (isRegular() && isAppended()) {
    auto appendedToID = l.settings.value("appendedTo").toULongLong();
    if ((0 >= appendedToID) || !l.objectIDToTrack.contains(appendedToID))
      throw InvalidSettingsX{};
    m_appendedTo = l.objectIDToTrack.value(appendedToID);
  }

  m_appendedTracks.clear();
  for (auto &appendedTrackID : l.settings.value("appendedTracks").toStringList()) {
    if (!l.objectIDToTrack.contains(appendedTrackID.toULongLong()))
      throw InvalidSettingsX{};
    m_appendedTracks << l.objectIDToTrack.value(appendedTrackID.toULongLong());
  }
}

std::string
Track::debugInfo()
  const {
  return fmt::format("{0}/{1}:{2}@{3}", static_cast<unsigned int>(m_type), m_id, m_codec, static_cast<void const *>(this));
}

void
Track::buildMkvmergeOptions(MkvmergeOptionBuilder &opt)
  const {
  ++opt.numTracksOfType[m_type];

  if (!m_muxThis || (m_appendedTo && !m_appendedTo->m_muxThis))
    return;

  auto sid = QString::number(mkvmergeTrackId());
  opt.enabledTrackIds[m_type] << sid;

  if (isTags() || isGlobalTags() || isAttachment())
    return;

  if (!m_delay.isEmpty() || !m_stretchBy.isEmpty())
    opt.options << Q("--sync") << Q("%1:%2").arg(sid).arg(MuxConfig::formatDelayAndStretchBy(m_delay, m_stretchBy));

  if (isAudio()) {
    if (m_aacSbrWasDetected || (0 != m_aacIsSBR))
      opt.options << Q("--aac-is-sbr") << Q("%1:%2").arg(sid).arg((1 == m_aacIsSBR) || ((0 == m_aacIsSBR) && m_aacSbrWasDetected) ? 1 : 0);

    if (canReduceToAudioCore() && m_reduceAudioToCore)
      opt.options << Q("--reduce-to-core") << sid;

    if (canRemoveDialogNormalizationGain() && m_removeDialogNormalizationGain)
      opt.options << Q("--remove-dialog-normalization-gain") << sid;

  } else if (isVideo()) {
    if (!m_cropping.isEmpty())
      opt.options << Q("--cropping") << Q("%1:%2").arg(sid).arg(m_cropping);

  } else if (isSubtitles()) {
    if (canChangeSubCharset() && !m_characterSet.isEmpty())
      opt.options << Q("--sub-charset") << Q("%1:%2").arg(sid).arg(m_characterSet);

  } else if (isChapters()) {
    if (!m_characterSet.isEmpty())
      opt.options << Q("--chapter-charset") << m_characterSet;

    if (m_language.is_valid())
      opt.options << Q("--chapter-language") << Q(m_language.format());

    return;

  }

  if (!m_appendedTo) {
    opt.options << Q("--language") << Q("%1:%2").arg(sid).arg(Q(m_language.format()));

    if (m_cues) {
      auto cues = 1 == m_cues ? Q(":iframes")
                : 2 == m_cues ? Q(":all")
                :               Q(":none");
      opt.options << Q("--cues") << (sid + cues);
    }

    if (!m_name.isEmpty() || m_nameWasPresent)
      opt.options << Q("--track-name") << Q("%1:%2").arg(sid).arg(m_name);

    if (m_defaultTrackFlagWasSet != m_defaultTrackFlag)
      opt.options << Q("--default-track-flag") << Q("%1:%2").arg(sid).arg(m_defaultTrackFlag ? Q("yes") : Q("no"));

    if (m_forcedTrackFlagWasSet != !!m_forcedTrackFlag)
      opt.options << Q("--forced-display-flag") << Q("%1:%2").arg(sid).arg(m_forcedTrackFlag == 1 ? Q("yes") : Q("no"));

    if (m_trackEnabledFlagWasSet != !!m_trackEnabledFlag)
      opt.options << Q("--track-enabled-flag") << Q("%1:%2").arg(sid).arg(m_trackEnabledFlag == 1 ? Q("yes") : Q("no"));

    if (m_hearingImpairedFlagWasSet != m_hearingImpairedFlag)
      opt.options << Q("--hearing-impaired-flag") << Q("%1:%2").arg(sid).arg(m_hearingImpairedFlag ? Q("yes") : Q("no"));

    if (m_visualImpairedFlagWasSet != m_visualImpairedFlag)
      opt.options << Q("--visual-impaired-flag") << Q("%1:%2").arg(sid).arg(m_visualImpairedFlag ? Q("yes") : Q("no"));

    if (m_textDescriptionsFlagWasSet != m_textDescriptionsFlag)
      opt.options << Q("--text-descriptions-flag") << Q("%1:%2").arg(sid).arg(m_textDescriptionsFlag ? Q("yes") : Q("no"));

    if (m_originalFlagWasSet != m_originalFlag)
      opt.options << Q("--original-flag") << Q("%1:%2").arg(sid).arg(m_originalFlag ? Q("yes") : Q("no"));

    if (m_commentaryFlagWasSet != m_commentaryFlag)
      opt.options << Q("--commentary-flag") << Q("%1:%2").arg(sid).arg(m_commentaryFlag ? Q("yes") : Q("no"));

    if (m_audioEmphasis != audio_emphasis_c::unspecified)
      opt.options << Q("--audio-emphasis") << Q("%1:%2").arg(sid).arg(Q(audio_emphasis_c::symbol(static_cast<int>(m_audioEmphasis))));

    if (!m_tags.isEmpty())
      opt.options << Q("--tags") << Util::CommandLineOption::fileName(Q("%1:%2").arg(sid).arg(m_tags));

    if (m_setAspectRatio && !m_aspectRatio.isEmpty())
      opt.options << Q("--aspect-ratio") << Q("%1:%2").arg(sid).arg(m_aspectRatio);

    else if (!m_setAspectRatio && !m_displayHeight.isEmpty() && !m_displayWidth.isEmpty())
      opt.options << Q("--display-dimensions") << Q("%1:%2x%3").arg(sid).arg(m_displayWidth).arg(m_displayHeight);

    if (m_stereoscopy)
      opt.options << Q("--stereo-mode") << Q("%1:%2").arg(sid).arg(m_stereoscopy - 1);

    if (!m_colorMatrixCoefficients.isEmpty())
      opt.options << Q("--color-matrix-coefficients") << Q("%1:%2").arg(sid).arg(m_colorMatrixCoefficients);

    if (!m_bitsPerColorChannel.isEmpty())
      opt.options << Q("--color-bits-per-channel") << Q("%1:%2").arg(sid).arg(m_bitsPerColorChannel);

    if (!m_chromaSubsampling.isEmpty())
      opt.options << Q("--chroma-subsample") << Q("%1:%2").arg(sid).arg(m_chromaSubsampling);

    if (!m_cbSubsampling.isEmpty())
      opt.options << Q("--cb-subsample") << Q("%1:%2").arg(sid).arg(m_cbSubsampling);

    if (!m_chromaSiting.isEmpty())
      opt.options << Q("--chroma-siting") << Q("%1:%2").arg(sid).arg(m_chromaSiting);

    if (!m_colorRange.isEmpty())
      opt.options << Q("--color-range") << Q("%1:%2").arg(sid).arg(m_colorRange);

    if (!m_transferCharacteristics.isEmpty())
      opt.options << Q("--color-transfer-characteristics") << Q("%1:%2").arg(sid).arg(m_transferCharacteristics);

    if (!m_colorPrimaries.isEmpty())
      opt.options << Q("--color-primaries") << Q("%1:%2").arg(sid).arg(m_colorPrimaries);

    if (!m_maximumContentLight.isEmpty())
      opt.options << Q("--max-content-light") << Q("%1:%2").arg(sid).arg(m_maximumContentLight);

    if (!m_maximumFrameLight.isEmpty())
      opt.options << Q("--max-frame-light") << Q("%1:%2").arg(sid).arg(m_maximumFrameLight);

    if (!m_chromaticityCoordinates.isEmpty())
      opt.options << Q("--chromaticity-coordinates") << Q("%1:%2").arg(sid).arg(m_chromaticityCoordinates);

    if (!m_whiteColorCoordinates.isEmpty())
      opt.options << Q("--white-color-coordinates") << Q("%1:%2").arg(sid).arg(m_whiteColorCoordinates);

    if (!m_maximumLuminance.isEmpty())
      opt.options << Q("--max-luminance") << Q("%1:%2").arg(sid).arg(m_maximumLuminance);

    if (!m_minimumLuminance.isEmpty())
      opt.options << Q("--min-luminance") << Q("%1:%2").arg(sid).arg(m_minimumLuminance);

    if (!m_projectionType.isEmpty())
      opt.options << Q("--projection-type") << Q("%1:%2").arg(sid).arg(m_projectionType);
    if (!m_projectionSpecificData.isEmpty())
      opt.options << Q("--projection-private") << Q("%1:%2").arg(sid).arg(m_projectionSpecificData);

    if (!m_yawRotation.isEmpty())
      opt.options << Q("--projection-pose-yaw") << Q("%1:%2").arg(sid).arg(m_yawRotation);

    if (!m_pitchRotation.isEmpty())
      opt.options << Q("--projection-pose-pitch") << Q("%1:%2").arg(sid).arg(m_pitchRotation);

    if (!m_rollRotation.isEmpty())
      opt.options << Q("--projection-pose-roll") << Q("%1:%2").arg(sid).arg(m_rollRotation);

    if (m_compression != TrackCompression::Default)
      opt.options << Q("--compression") << Q("%1:%2").arg(sid).arg(TrackCompression::None == m_compression ? Q("none") : Q("zlib"));
  }

  if (!m_defaultDuration.isEmpty()) {
    auto unit = m_defaultDuration.contains(QRegularExpression{"\\d$"}) ? Q("fps") : Q("");
    opt.options << Q("--default-duration") << Q("%1:%2%3").arg(sid).arg(m_defaultDuration).arg(unit);
  }

  if (!m_timestamps.isEmpty())
    opt.options << Q("--timestamps") << Util::CommandLineOption::fileName(Q("%1:%2").arg(sid).arg(m_timestamps));

  if (m_fixBitstreamTimingInfo)
    opt.options << Q("--fix-bitstream-timing-information") << Q("%1:1").arg(sid);

  auto additionalOptions = Q(mtx::string::strip_copy(to_utf8(m_additionalOptions)));
  if (!additionalOptions.isEmpty())
    opt.options << additionalOptions.replace(Q("<TID>"), sid).split(QRegularExpression{" +"});
}

QString
Track::nameForType(TrackType type) {
  return TrackType::Audio      == type ? QY("Audio")
       : TrackType::Video      == type ? QY("Video")
       : TrackType::Subtitles  == type ? QY("Subtitles")
       : TrackType::Buttons    == type ? QY("Buttons")
       : TrackType::Attachment == type ? QY("Attachment")
       : TrackType::Chapters   == type ? QY("Chapters")
       : TrackType::Tags       == type ? QY("Tags")
       : TrackType::GlobalTags == type ? QY("Global tags")
       :                                 Q("INTERNAL ERROR");
}

QString
Track::nameForType()
  const {
  return nameForType(m_type);
}

bool
Track::canChangeSubCharset()
  const {
  if (   isSubtitles()
      && m_properties.value(Q("text_subtitles")).toBool()
      && (   m_properties.value(Q("encoding")).toString().isEmpty()
          || mtx::included_in(m_file->m_type, mtx::file_type_e::matroska, mtx::file_type_e::mpeg_ts)))
    return true;

  if (   isChapters()
       && mtx::included_in(m_file->m_type, mtx::file_type_e::qtmp4, mtx::file_type_e::mpeg_ts, mtx::file_type_e::ogm))
    return true;

  return false;
}

bool
Track::canReduceToAudioCore()
  const {
  return isAudio()
      && m_codec.contains(QRegularExpression{Q("dts"), QRegularExpression::CaseInsensitiveOption});
}

bool
Track::canRemoveDialogNormalizationGain()
  const {
  return isAudio()
      && m_codec.contains(QRegularExpression{Q("ac-?3|dts|truehd"), QRegularExpression::CaseInsensitiveOption});
}

bool
Track::canSetAacToSbr()
  const {
  return isAudio()
      && m_codec.contains(QRegularExpression{Q("aac"), QRegularExpression::CaseInsensitiveOption})
      && mtx::included_in(m_file->m_type, mtx::file_type_e::aac, mtx::file_type_e::matroska, mtx::file_type_e::real);
}

int
Track::mkvmergeTrackId()
  const {
  if (isChapters())
    return -2;

  return m_id;
}

}
