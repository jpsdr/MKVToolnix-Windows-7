/**
   mkvpropedit -- utility for editing properties of existing Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   \file

   \author Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <string>
#include <vector>

#include <ebml/EbmlBinary.h>
#include <ebml/EbmlFloat.h>
#include <ebml/EbmlSInteger.h>
#include <ebml/EbmlString.h>
#include <ebml/EbmlUInteger.h>
#include <ebml/EbmlUnicodeString.h>

#if LIBMATROSKA_VERSION < 0x020000
# include <matroska/KaxInfoData.h>
#endif
#include <matroska/KaxTracks.h>

#include "common/common_pch.h"
#include "common/ebml.h"
#include "common/list_utils.h"
#include "common/property_element.h"
#include "common/translation.h"



std::map<uint32_t, std::vector<property_element_c> > property_element_c::s_properties;
std::map<uint32_t, std::vector<property_element_c> > property_element_c::s_composed_properties;
std::unordered_map<std::string, std::string>         property_element_c::s_aliases;

property_element_c::property_element_c(std::string name,
                                       libebml::EbmlCallbacks const &callbacks,
                                       translatable_string_c title,
                                       translatable_string_c description,
                                       libebml::EbmlCallbacks const &sub_master_callbacks,
                                       libebml::EbmlCallbacks const *sub_sub_master_callbacks,
                                       libebml::EbmlCallbacks const *sub_sub_sub_master_callbacks)
  : m_name{std::move(name)}
  , m_title{std::move(title)}
  , m_description{std::move(description)}
  , m_callbacks{&callbacks}
  , m_sub_master_callbacks{&sub_master_callbacks}
  , m_sub_sub_master_callbacks{sub_sub_master_callbacks}
  , m_sub_sub_sub_master_callbacks{sub_sub_sub_master_callbacks}
  , m_bit_length{128}
  , m_type{EBMLT_SKIP}
{
  derive_type();
}

property_element_c::property_element_c()
  : m_callbacks(nullptr)
  , m_sub_master_callbacks(nullptr)
  , m_type(EBMLT_SKIP)
{
}

bool
property_element_c::is_valid()
  const
{
  return !m_name.empty() && (m_callbacks) && (EBMLT_SKIP != m_type);
}

void
property_element_c::derive_type() {
  auto e_ptr = std::unique_ptr<libebml::EbmlElement>(&m_callbacks->NewElement());
  auto e     = e_ptr.get();

  m_type = dynamic_cast<libebml::EbmlBinary *>(e)        ? EBMLT_BINARY
         : dynamic_cast<libebml::EbmlFloat *>(e)         ? EBMLT_FLOAT
         : dynamic_cast<libebml::EbmlSInteger *>(e)      ? EBMLT_INT
         : dynamic_cast<libebml::EbmlString *>(e)        ? EBMLT_STRING
         : dynamic_cast<libebml::EbmlUInteger *>(e)      ? EBMLT_UINT
         : dynamic_cast<libebml::EbmlUnicodeString *>(e) ? EBMLT_USTRING
         : dynamic_cast<libebml::EbmlDate *>(e)          ? EBMLT_DATE
         :                                                 EBMLT_SKIP;

  if (EBMLT_SKIP == m_type)
    mxerror(fmt::format("property_element_c::derive_type(): programming error: unknown type for EBML ID {0:08x}\n", m_callbacks->ClassId().GetValue()));

  if (   (EBMLT_UINT == m_type)
      && (   (m_name.find("flag") != std::string::npos)
          || mtx::included_in(m_name, "alpha-mode")))
    m_type = EBMLT_BOOL;
}

void
property_element_c::init_tables() {
  libebml::EbmlCallbacks const *sub_master_callbacks = nullptr, *sub_sub_master_callbacks = nullptr, *sub_sub_sub_master_callbacks = nullptr;

  s_properties.clear();

  s_properties[EBML_ID(libmatroska::KaxInfo).GetValue()]   = std::vector<property_element_c>();
  s_properties[EBML_ID(libmatroska::KaxTracks).GetValue()] = std::vector<property_element_c>();
  uint32_t current_index                                   = EBML_ID(libmatroska::KaxInfo).GetValue();

  auto look_up = [](libebml::EbmlId const &id, std::string const &name) -> property_element_c & {
    auto &props = s_properties[id.GetValue()];
    auto itr    = std::find_if(props.begin(), props.end(), [&name](auto const &prop) { return prop.m_name == name; });
    return *itr;
  };

  auto add = [&current_index, &sub_master_callbacks, &sub_sub_master_callbacks, &sub_sub_sub_master_callbacks]
    (char const *name, libebml::EbmlCallbacks const &callbacks, translatable_string_c const &title, translatable_string_c const &description) {
    s_properties[current_index].push_back(property_element_c(name, callbacks, title, description, *sub_master_callbacks, sub_sub_master_callbacks, sub_sub_sub_master_callbacks));
  };

  add("title",                EBML_INFO(libmatroska::KaxTitle),           YT("Title"),                        YT("The title for the whole movie."));
  add("date",                 EBML_INFO(libmatroska::KaxDateUTC),         YT("Date"),                         YT("The date the file was created."));
  add("segment-filename",     EBML_INFO(libmatroska::KaxSegmentFilename), YT("Segment filename"),             YT("The file name for this segment."));
  add("prev-filename",        EBML_INFO(libmatroska::KaxPrevFilename),    YT("Previous filename"),            YT("An escaped filename corresponding to the previous segment."));
  add("next-filename",        EBML_INFO(libmatroska::KaxNextFilename),    YT("Next filename"),                YT("An escaped filename corresponding to the next segment."));
  add("segment-uid",          EBML_INFO(libmatroska::KaxSegmentUID),      YT("Segment unique ID"),            YT("A randomly generated unique ID to identify the current segment between many others (128 bits)."));
  add("prev-uid",             EBML_INFO(libmatroska::KaxPrevUID),         YT("Previous segment's unique ID"), YT("A unique ID to identify the previous chained segment (128 bits)."));
  add("next-uid",             EBML_INFO(libmatroska::KaxNextUID),         YT("Next segment's unique ID"),     YT("A unique ID to identify the next chained segment (128 bits)."));
  add("muxing-application",   EBML_INFO(libmatroska::KaxMuxingApp),       YT("Multiplexing application"),     YT("The name of the application or library used for multiplexing the file."));
  add("writing-application",  EBML_INFO(libmatroska::KaxWritingApp),      YT("Writing application"),          YT("The name of the application or library used for writing the file."));

  current_index = EBML_ID(libmatroska::KaxTracks).GetValue();

  add("track-number",           EBML_INFO(libmatroska::KaxTrackNumber),          YT("Track number"),               YT("The track number as used in the Block Header."));
  add("track-uid",              EBML_INFO(libmatroska::KaxTrackUID),             YT("Track UID"),                  YT("A unique ID to identify the Track. This should be kept the same when making a direct stream copy of the Track to another file."));
  add("flag-commentary",        EBML_INFO(libmatroska::KaxFlagCommentary),       YT("\"Commentary\" flag"),        YT("Can be set if the track contains commentary."));
  add("flag-default",           EBML_INFO(libmatroska::KaxTrackFlagDefault),     YT("\"Default track\" flag"),     YT("Set if that track (audio, video or subs) SHOULD be used if no language found matches the user preference."));
  add("flag-enabled",           EBML_INFO(libmatroska::KaxTrackFlagEnabled),     YT("\"Track enabled\" flag"),     YT("Set if the track is used."));
  add("flag-forced",            EBML_INFO(libmatroska::KaxTrackFlagForced),      YT("\"Forced display\" flag"),    YT("Can be set for tracks containing onscreen text or foreign-language dialog."));
  add("flag-hearing-impaired",  EBML_INFO(libmatroska::KaxFlagHearingImpaired),  YT("\"Hearing impaired\" flag"),  YT("Can be set if the track is suitable for users with hearing impairments."));
  add("flag-original",          EBML_INFO(libmatroska::KaxFlagOriginal),         YT("\"Original language\" flag"), YT("Can be set if the track is in the content's original language (not a translation)."));
  add("flag-text-descriptions", EBML_INFO(libmatroska::KaxFlagTextDescriptions), YT("\"Text descriptions\" flag"), YT("Can be set if the track contains textual descriptions of video content suitable for playback via a text-to-speech system for a visually-impaired user."));
  add("flag-visual-impaired",   EBML_INFO(libmatroska::KaxFlagVisualImpaired),   YT("\"Visual impaired\" flag"),   YT("Can be set if the track is suitable for users with visual impairments."));
  add("default-duration",       EBML_INFO(libmatroska::KaxTrackDefaultDuration), YT("Default duration"),           YT("Number of nanoseconds (not scaled) per frame."));
  add("name",                   EBML_INFO(libmatroska::KaxTrackName),            YT("Name"),                       YT("A human-readable track name."));
  add("language",               EBML_INFO(libmatroska::KaxTrackLanguage),        YT("Language (obsolete)"),        YT("Specifies the language of the track (obsolete)."));
  add("language-ietf",          EBML_INFO(libmatroska::KaxLanguageIETF),         YT("Language"),                   YT("Specifies the language of the track in the form of a BCP 47 language tag."));
  add("codec-id",               EBML_INFO(libmatroska::KaxCodecID),              YT("Codec ID"),                   YT("An ID corresponding to the codec."));
  add("codec-name",             EBML_INFO(libmatroska::KaxCodecName),            YT("Codec name"),                 YT("A human-readable string specifying the codec."));
  add("codec-delay",            EBML_INFO(libmatroska::KaxCodecDelay),           YT("Codec-inherent delay"),       YT("Delay built into the codec during decoding in ns."));

  sub_master_callbacks = &EBML_INFO(libmatroska::KaxTrackVideo);

  add("alpha-mode",        EBML_INFO(libmatroska::KaxVideoAlphaMode),       YT("Video alpha mode"),        YT("Set if the BlockAdditional element with BlockAddID of '1' contains alpha channel data."));
  add("interlaced",        EBML_INFO(libmatroska::KaxVideoFlagInterlaced),  YT("Video interlaced flag"),   YT("Set if the video is interlaced."));
  add("pixel-width",       EBML_INFO(libmatroska::KaxVideoPixelWidth),      YT("Video pixel width"),       YT("Width of the encoded video frames in pixels."));
  add("pixel-height",      EBML_INFO(libmatroska::KaxVideoPixelHeight),     YT("Video pixel height"),      YT("Height of the encoded video frames in pixels."));
  add("display-width",     EBML_INFO(libmatroska::KaxVideoDisplayWidth),    YT("Video display width"),     YT("Width of the video frames to display."));
  add("display-height",    EBML_INFO(libmatroska::KaxVideoDisplayHeight),   YT("Video display height"),    YT("Height of the video frames to display."));
  add("display-unit",      EBML_INFO(libmatroska::KaxVideoDisplayUnit),     YT("Video display unit"),      YT("Type of the unit for DisplayWidth/Height (0: pixels, 1: centimeters, 2: inches, 3: aspect ratio)."));
  add("pixel-crop-left",   EBML_INFO(libmatroska::KaxVideoPixelCropLeft),   YT("Video crop left"),         YT("The number of video pixels to remove on the left of the image."));
  add("pixel-crop-top",    EBML_INFO(libmatroska::KaxVideoPixelCropTop),    YT("Video crop top"),          YT("The number of video pixels to remove on the top of the image."));
  add("pixel-crop-right",  EBML_INFO(libmatroska::KaxVideoPixelCropRight),  YT("Video crop right"),        YT("The number of video pixels to remove on the right of the image."));
  add("pixel-crop-bottom", EBML_INFO(libmatroska::KaxVideoPixelCropBottom), YT("Video crop bottom"),       YT("The number of video pixels to remove on the bottom of the image."));
  add("aspect-ratio-type", EBML_INFO(libmatroska::KaxVideoAspectRatio),     YT("Video aspect ratio type"), YT("Specify the possible modifications to the aspect ratio (0: free resizing, 1: keep aspect ratio, 2: fixed)."));
  add("field-order",       EBML_INFO(libmatroska::KaxVideoFieldOrder),      YT("Video field order"),       YT("Field order (0, 1, 2, 6, 9 or 14, see documentation)."));
  add("stereo-mode",       EBML_INFO(libmatroska::KaxVideoStereoMode),      YT("Video stereo mode"),       YT("Stereo-3D video mode (0 - 14, see documentation)."));

  sub_master_callbacks     = &EBML_INFO(libmatroska::KaxTrackVideo);
  sub_sub_master_callbacks = &EBML_INFO(libmatroska::KaxVideoColour);

  add("color-matrix-coefficients",      EBML_INFO(libmatroska::KaxVideoColourMatrix),            YT("Video: color matrix coefficients"),     YT("Sets the matrix coefficients of the video used to derive luma and chroma values from red, green and blue color primaries."));
  add("color-bits-per-channel",         EBML_INFO(libmatroska::KaxVideoBitsPerChannel),          YT("Video: bits per color channel"),        YT("Sets the number of coded bits for a color channel."));
  add("chroma-subsample-horizontal",    EBML_INFO(libmatroska::KaxVideoChromaSubsampHorz),       YT("Video: horizontal chroma subsampling"), YT("The amount of pixels to remove in the Cr and Cb channels for every pixel not removed horizontally."));
  add("chroma-subsample-vertical",      EBML_INFO(libmatroska::KaxVideoChromaSubsampVert),       YT("Video: vertical chroma subsampling"),   YT("The amount of pixels to remove in the Cr and Cb channels for every pixel not removed vertically."));
  add("cb-subsample-horizontal",        EBML_INFO(libmatroska::KaxVideoCbSubsampHorz),           YT("Video: horizontal Cb subsampling"),     YT("The amount of pixels to remove in the Cb channel for every pixel not removed horizontally. This is additive with chroma-subsample-horizontal."));
  add("cb-subsample-vertical",          EBML_INFO(libmatroska::KaxVideoCbSubsampVert),           YT("Video: vertical Cb subsampling"),       YT("The amount of pixels to remove in the Cb channel for every pixel not removed vertically. This is additive with chroma-subsample-vertical."));
  add("chroma-siting-horizontal",       EBML_INFO(libmatroska::KaxVideoChromaSitHorz),           YT("Video: horizontal chroma siting"),      YT("How chroma is sited horizontally."));
  add("chroma-siting-vertical",         EBML_INFO(libmatroska::KaxVideoChromaSitVert),           YT("Video: vertical chroma siting"),        YT("How chroma is sited vertically."));
  add("color-range",                    EBML_INFO(libmatroska::KaxVideoColourRange),             YT("Video: color range"),                   YT("Clipping of the color ranges."));
  add("color-transfer-characteristics", EBML_INFO(libmatroska::KaxVideoColourTransferCharacter), YT("Video: transfer characteristics"),      YT("The color transfer characteristics of the video."));
  add("color-primaries",                EBML_INFO(libmatroska::KaxVideoColourPrimaries),         YT("Video: color primaries"),               YT("The color primaries of the video."));
  add("max-content-light",              EBML_INFO(libmatroska::KaxVideoColourMaxCLL),            YT("Video: maximum content light"),         YT("Maximum brightness of a single pixel in candelas per square meter (cd/m²)."));
  add("max-frame-light",                EBML_INFO(libmatroska::KaxVideoColourMaxFALL),           YT("Video: maximum frame light"),           YT("Maximum frame-average light level in candelas per square meter (cd/m²)."));

  sub_sub_sub_master_callbacks = &EBML_INFO(libmatroska::KaxVideoColourMasterMeta);

  add("chromaticity-coordinates-red-x",   EBML_INFO(libmatroska::KaxVideoRChromaX),                YT("Video: chromaticity red X"),         YT("Red X chromaticity coordinate as defined by CIE 1931."));
  add("chromaticity-coordinates-red-y",   EBML_INFO(libmatroska::KaxVideoRChromaY),                YT("Video: chromaticity red Y"),         YT("Red Y chromaticity coordinate as defined by CIE 1931."));
  add("chromaticity-coordinates-green-x", EBML_INFO(libmatroska::KaxVideoGChromaX),                YT("Video: chromaticity green X"),       YT("Green X chromaticity coordinate as defined by CIE 1931."));
  add("chromaticity-coordinates-green-y", EBML_INFO(libmatroska::KaxVideoGChromaY),                YT("Video: chromaticity green Y"),       YT("Green Y chromaticity coordinate as defined by CIE 1931."));
  add("chromaticity-coordinates-blue-x",  EBML_INFO(libmatroska::KaxVideoBChromaX),                YT("Video: chromaticity blue X"),        YT("Blue X chromaticity coordinate as defined by CIE 1931."));
  add("chromaticity-coordinates-blue-y",  EBML_INFO(libmatroska::KaxVideoBChromaY),                YT("Video: chromaticity blue Y"),        YT("Blue Y chromaticity coordinate as defined by CIE 1931."));
  add("white-coordinates-x",              EBML_INFO(libmatroska::KaxVideoWhitePointChromaX),       YT("Video: white point X"),              YT("White color chromaticity coordinate X as defined by CIE 1931."));
  add("white-coordinates-y",              EBML_INFO(libmatroska::KaxVideoWhitePointChromaY),       YT("Video: white point Y"),              YT("White color chromaticity coordinate Y as defined by CIE 1931."));
  add("max-luminance",                    EBML_INFO(libmatroska::KaxVideoLuminanceMax),            YT("Video: maximum luminance"),          YT("Maximum luminance in candelas per square meter (cd/m²)."));
  add("min-luminance",                    EBML_INFO(libmatroska::KaxVideoLuminanceMin),            YT("Video: minimum luminance"),          YT("Minimum luminance in candelas per square meter (cd/m²)."));

  sub_sub_master_callbacks     = &EBML_INFO(libmatroska::KaxVideoProjection);
  sub_sub_sub_master_callbacks = nullptr;

  add("projection-type",       EBML_INFO(libmatroska::KaxVideoProjectionType),      YT("Video: projection type"),             YT("Describes the projection used for this video track (0 – 3)."));
  add("projection-private",    EBML_INFO(libmatroska::KaxVideoProjectionPrivate),   YT("Video: projection-specific data"),    YT("Private data that only applies to a specific projection."));
  add("projection-pose-yaw",   EBML_INFO(libmatroska::KaxVideoProjectionPoseYaw),   YT("Video: projection's yaw rotation"),   YT("Specifies a yaw rotation to the projection."));
  add("projection-pose-pitch", EBML_INFO(libmatroska::KaxVideoProjectionPosePitch), YT("Video: projection's pitch rotation"), YT("Specifies a pitch rotation to the projection."));
  add("projection-pose-roll",  EBML_INFO(libmatroska::KaxVideoProjectionPoseRoll),  YT("Video: projection's roll rotation"),  YT("Specifies a roll rotation to the projection."));

  sub_master_callbacks         = &EBML_INFO(libmatroska::KaxTrackAudio);
  sub_sub_master_callbacks     = nullptr;
  sub_sub_sub_master_callbacks = nullptr;

  add("sampling-frequency",        EBML_INFO(libmatroska::KaxAudioSamplingFreq),       YT("Audio sampling frequency"),        YT("Sampling frequency in Hz."));
  add("output-sampling-frequency", EBML_INFO(libmatroska::KaxAudioOutputSamplingFreq), YT("Audio output sampling frequency"), YT("Real output sampling frequency in Hz."));
  add("channels",                  EBML_INFO(libmatroska::KaxAudioChannels),           YT("Audio channels"),                  YT("Numbers of channels in the track."));
  add("bit-depth",                 EBML_INFO(libmatroska::KaxAudioBitDepth),           YT("Audio bit depth"),                 YT("Bits per sample, mostly used for PCM."));
  add("emphasis",                  EBML_INFO(libmatroska::KaxEmphasis),                YT("Audio emphasis"),                  YT("Emphasis applied on audio which must be reversed by player to get the actual samples."));

  look_up(EBML_ID(libmatroska::KaxTracks), "projection-private").m_bit_length = 0;

  // Aliases for old names.
  s_aliases["colour-matrix-coefficients"]      = "color-matrix-coefficients";
  s_aliases["colour-bits-per-channel"]         = "color-bits-per-channel";
  s_aliases["colour-range"]                    = "color-range";
  s_aliases["colour-transfer-characteristics"] = "color-transfer-characteristics";
  s_aliases["colour-primaries"]                = "color-primaries";
}

std::vector<property_element_c> &
property_element_c::get_table_for(const libebml::EbmlCallbacks &master_callbacks,
                                  const libebml::EbmlCallbacks *sub_master_callbacks,
                                  bool full_table) {
  if (s_properties.empty())
    init_tables();

  auto src_map_it = s_properties.find(master_callbacks.ClassId().GetValue());
  if (s_properties.end() == src_map_it)
    mxerror(fmt::format("property_element_c::get_table_for(): programming error: no table found for EBML ID {0:08x}\n", master_callbacks.ClassId().GetValue()));

  if (full_table)
    return src_map_it->second;

  uint32_t element_id  = !sub_master_callbacks ? master_callbacks.ClassId().GetValue() : sub_master_callbacks->ClassId().GetValue();
  auto composed_map_it = s_composed_properties.find(element_id);
  if (s_composed_properties.end() != composed_map_it)
    return composed_map_it->second;

  s_composed_properties[element_id]      = std::vector<property_element_c>();
  std::vector<property_element_c> &table = s_composed_properties[element_id];

  for (auto &property : src_map_it->second)
    if (!property.m_sub_master_callbacks || (sub_master_callbacks && (sub_master_callbacks->ClassId() == property.m_sub_master_callbacks->ClassId())))
      table.push_back(property);

  return table;
}

std::string
property_element_c::get_actual_name(std::string const &name) {
  auto itr = s_aliases.find(name);
  return itr == s_aliases.end() ? name : itr->second;
}
