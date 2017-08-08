/**
   mkvpropedit -- utility for editing properties of existing Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   \file

   \author Written by Moritz Bunkus <moritz@bunkus.org>.
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
#include <matroska/KaxInfo.h>
#include <matroska/KaxInfoData.h>
#include <matroska/KaxTracks.h>
#include <matroska/KaxTrackAudio.h>
#include <matroska/KaxTrackEntryData.h>
#include <matroska/KaxTrackVideo.h>

#include "common/common_pch.h"
#include "common/ebml.h"
#include "common/property_element.h"
#include "common/translation.h"

std::map<uint32_t, std::vector<property_element_c> > property_element_c::s_properties;
std::map<uint32_t, std::vector<property_element_c> > property_element_c::s_composed_properties;

property_element_c::property_element_c(std::string const &name,
                                       EbmlCallbacks const &callbacks,
                                       translatable_string_c const &title,
                                       translatable_string_c const &description,
                                       EbmlCallbacks const &sub_master_callbacks,
                                       EbmlCallbacks const *sub_sub_master_callbacks,
                                       EbmlCallbacks const *sub_sub_sub_master_callbacks)
  : m_name{name}
  , m_title{title}
  , m_description{description}
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
  EbmlElement *e = &m_callbacks->Create();

  m_type = dynamic_cast<EbmlBinary *>(e)        ? EBMLT_BINARY
         : dynamic_cast<EbmlFloat *>(e)         ? EBMLT_FLOAT
         : dynamic_cast<EbmlSInteger *>(e)      ? EBMLT_INT
         : dynamic_cast<EbmlString *>(e)        ? EBMLT_STRING
         : dynamic_cast<EbmlUInteger *>(e)      ? EBMLT_UINT
         : dynamic_cast<EbmlUnicodeString *>(e) ? EBMLT_USTRING
         : dynamic_cast<EbmlDate *>(e)          ? EBMLT_DATE
         :                                        EBMLT_SKIP;

  if (EBMLT_SKIP == m_type)
    mxerror(boost::format("property_element_c::derive_type(): programming error: unknown type for EBML ID %|1$08x|\n") % m_callbacks->GlobalId.Value);

  if ((EBMLT_UINT == m_type) && (m_name.find("flag") != std::string::npos))
    m_type = EBMLT_BOOL;

  delete e;
}

#define ELE( name, callbacks, title, description) s_properties[current_index].push_back(property_element_c(name, callbacks, title, description, *sub_master_callbacks, nullptr, nullptr))
#define ELE2(name, callbacks, title, description) s_properties[current_index].push_back(property_element_c(name, callbacks, title, description, *sub_master_callbacks, sub_sub_master_callbacks))
#define ELE3(name, callbacks, title, description) s_properties[current_index].push_back(property_element_c(name, callbacks, title, description, *sub_master_callbacks, sub_sub_master_callbacks, sub_sub_sub_master_callbacks))

void
property_element_c::init_tables() {
  EbmlCallbacks const *sub_master_callbacks = nullptr, *sub_sub_master_callbacks = nullptr, *sub_sub_sub_master_callbacks = nullptr;

  s_properties.clear();

  s_properties[KaxInfo::ClassInfos.GlobalId.Value]   = std::vector<property_element_c>();
  s_properties[KaxTracks::ClassInfos.GlobalId.Value] = std::vector<property_element_c>();
  uint32_t current_index                             = KaxInfo::ClassInfos.GlobalId.Value;

  ELE("title",                KaxTitle::ClassInfos,           Y("Title"),                        Y("The title for the whole movie."));
  ELE("date",                 KaxDateUTC::ClassInfos,         Y("Date"),                         Y("The date the file was created."));
  ELE("segment-filename",     KaxSegmentFilename::ClassInfos, Y("Segment filename"),             Y("The file name for this segment."));
  ELE("prev-filename",        KaxPrevFilename::ClassInfos,    Y("Previous filename"),            Y("An escaped filename corresponding to\nthe previous segment."));
  ELE("next-filename",        KaxNextFilename::ClassInfos,    Y("Next filename"),                Y("An escaped filename corresponding to\nthe next segment."));
  ELE("segment-uid",          KaxSegmentUID::ClassInfos,      Y("Segment unique ID"),            Y("A randomly generated unique ID to identify the current\n"
                                                                                                   "segment between many others (128 bits)."));
  ELE("prev-uid",             KaxPrevUID::ClassInfos,         Y("Previous segment's unique ID"), Y("A unique ID to identify the previous chained\nsegment (128 bits)."));
  ELE("next-uid",             KaxNextUID::ClassInfos,         Y("Next segment's unique ID"),     Y("A unique ID to identify the next chained\nsegment (128 bits)."));
  ELE("muxing-application",   KaxMuxingApp::ClassInfos,       Y("Multiplexing application"),     Y("The name of the application or library used for multiplexing the file."));
  ELE("writing-application",  KaxWritingApp::ClassInfos,      Y("Writing application"),          Y("The name of the application or library used for writing the file."));

  current_index = KaxTracks::ClassInfos.GlobalId.Value;

  ELE("track-number",         KaxTrackNumber::ClassInfos,          Y("Track number"),          Y("The track number as used in the Block Header."));
  ELE("track-uid",            KaxTrackUID::ClassInfos,             Y("Track UID"),             Y("A unique ID to identify the Track. This should be\nkept the same when making a "
                                                                                                 "direct stream copy\nof the Track to another file."));
  ELE("flag-default",         KaxTrackFlagDefault::ClassInfos,     Y("'Default track' flag"),  Y("Set if that track (audio, video or subs) SHOULD\nbe used if no language found matches the\n"
                                                                                                 "user preference."));
  ELE("flag-enabled",         KaxTrackFlagEnabled::ClassInfos,     Y("'Track enabled' flag"),  Y("Set if the track is used."));
  ELE("flag-forced",          KaxTrackFlagForced::ClassInfos,      Y("'Forced display' flag"), Y("Set if that track MUST be used during playback.\n"
                                                                                                 "There can be many forced track for a kind (audio,\nvideo or subs). "
                                                                                                 "The player should select the one\nwhose language matches the user preference or the\n"
                                                                                                 "default + forced track."));
  ELE("min-cache",            KaxTrackMinCache::ClassInfos,        Y("Minimum cache"),         Y("The minimum number of frames a player\nshould be able to cache during playback.\n"
                                                                                                 "If set to 0, the reference pseudo-cache system\nis not used."));
  ELE("max-cache",            KaxTrackMaxCache::ClassInfos,        Y("Maximum cache"),         Y("The maximum number of frames a player\nshould be able to cache during playback.\n"
                                                                                                 "If set to 0, the reference pseudo-cache system\nis not used."));
  ELE("default-duration",     KaxTrackDefaultDuration::ClassInfos, Y("Default duration"),      Y("Number of nanoseconds (not scaled) per frame."));
  ELE("name",                 KaxTrackName::ClassInfos,            Y("Name"),                  Y("A human-readable track name."));
  ELE("language",             KaxTrackLanguage::ClassInfos,        Y("Language"),              Y("Specifies the language of the track in the\nMatroska languages form."));
  ELE("codec-id",             KaxCodecID::ClassInfos,              Y("Codec ID"),              Y("An ID corresponding to the codec."));
  ELE("codec-name",           KaxCodecName::ClassInfos,            Y("Codec name"),            Y("A human-readable string specifying the codec."));
  ELE("codec-delay",          KaxCodecDelay::ClassInfos,           Y("Codec-inherent delay"),  Y("Delay built into the codec during decoding in ns."));

  sub_master_callbacks = &KaxTrackVideo::ClassInfos;

  ELE("interlaced",        KaxVideoFlagInterlaced::ClassInfos,  Y("Video interlaced flag"),   Y("Set if the video is interlaced."));
  ELE("pixel-width",       KaxVideoPixelWidth::ClassInfos,      Y("Video pixel width"),       Y("Width of the encoded video frames in pixels."));
  ELE("pixel-height",      KaxVideoPixelHeight::ClassInfos,     Y("Video pixel height"),      Y("Height of the encoded video frames in pixels."));
  ELE("display-width",     KaxVideoDisplayWidth::ClassInfos,    Y("Video display width"),     Y("Width of the video frames to display."));
  ELE("display-height",    KaxVideoDisplayHeight::ClassInfos,   Y("Video display height"),    Y("Height of the video frames to display."));
  ELE("display-unit",      KaxVideoDisplayUnit::ClassInfos,     Y("Video display unit"),      Y("Type of the unit for DisplayWidth/Height\n(0: pixels, 1: centimeters, 2: inches, 3: aspect ratio)."));
  ELE("pixel-crop-left",   KaxVideoPixelCropLeft::ClassInfos,   Y("Video crop left"),         Y("The number of video pixels to remove\non the left of the image."));
  ELE("pixel-crop-top",    KaxVideoPixelCropTop::ClassInfos,    Y("Video crop top"),          Y("The number of video pixels to remove\non the top of the image."));
  ELE("pixel-crop-right",  KaxVideoPixelCropRight::ClassInfos,  Y("Video crop right"),        Y("The number of video pixels to remove\non the right of the image."));
  ELE("pixel-crop-bottom", KaxVideoPixelCropBottom::ClassInfos, Y("Video crop bottom"),       Y("The number of video pixels to remove\non the bottom of the image."));
  ELE("aspect-ratio-type", KaxVideoAspectRatio::ClassInfos,     Y("Video aspect ratio type"), Y("Specify the possible modifications to the aspect ratio\n"
                                                                                                "(0: free resizing, 1: keep aspect ratio, 2: fixed)."));
  ELE("field-order",       KaxVideoFieldOrder::ClassInfos,      Y("Video field order"),       Y("Field order (0, 1, 2, 6, 9 or 14, see documentation)."));
  ELE("stereo-mode",       KaxVideoStereoMode::ClassInfos,      Y("Video stereo mode"),       Y("Stereo-3D video mode (0 - 11, see documentation)."));

  sub_master_callbacks         = &KaxTrackVideo::ClassInfos;
  sub_sub_master_callbacks     = &KaxVideoColour::ClassInfos;
  sub_sub_sub_master_callbacks = &KaxVideoColourMasterMeta::ClassInfos;

  ELE2("colour-matrix-coefficients",       KaxVideoColourMatrix::ClassInfos,            Y("Video: colour matrix coefficients"), Y("Sets the matrix coefficients of the video used to derive luma and chroma values "
                                                                                                                                  "from red, green and blue color primaries."));
  ELE2("colour-bits-per-channel",          KaxVideoBitsPerChannel::ClassInfos,          Y("Video: bits per colour channel"),    Y("Sets the number of coded bits for a colour channel."));
  ELE2("chroma-subsample-horizontal",      KaxVideoChromaSubsampHorz::ClassInfos,       Y("Video: pixels to remove in chroma"), Y("The amount of pixels to remove in the Cr and Cb channels for every pixel not removed horizontally."));
  ELE2("chroma-subsample-vertical",        KaxVideoChromaSubsampVert::ClassInfos,       Y("Video: pixels to remove in chroma"), Y("The amount of pixels to remove in the Cr and Cb channels for every pixel not removed vertically."));
  ELE2("cb-subsample-horizontal",          KaxVideoCbSubsampHorz::ClassInfos,           Y("Video: pixels to remove in Cb"),     Y("The amount of pixels to remove in the Cb channel for every pixel not removed horizontally. "
                                                                                                                                  "This is additive with chroma-subsample-horizontal."));
  ELE2("cb-subsample-vertical",            KaxVideoCbSubsampVert::ClassInfos,           Y("Video: pixels to remove in Cb"),     Y("The amount of pixels to remove in the Cb channel for every pixel not removed vertically. "
                                                                                                                                  "This is additive with chroma-subsample-vertical."));
  ELE2("chroma-siting-horizontal",         KaxVideoChromaSitHorz::ClassInfos,           Y("Video: chroma siting"),              Y("How chroma is sited horizontally."));
  ELE2("chroma-siting-vertical",           KaxVideoChromaSitVert::ClassInfos,           Y("Video: chroma siting"),              Y("How chroma is sited vertically."));
  ELE2("colour-range",                     KaxVideoColourRange::ClassInfos,             Y("Video: colour range"),               Y("Clipping of the color ranges."));
  ELE2("colour-transfer-characteristics",  KaxVideoColourTransferCharacter::ClassInfos, Y("Video: transfer characteristics"),   Y("The colour transfer characteristics of the video."));
  ELE2("colour-primaries",                 KaxVideoColourPrimaries::ClassInfos,         Y("Video: colour primaries"),           Y("The colour primaries of the video."));
  ELE2("max-content-light",                KaxVideoColourMaxCLL::ClassInfos,            Y("Video: maximum content light"),      Y("Maximum brightness of a single pixel in candelas per square meter (cd/m²)."));
  ELE2("max-frame-light",                  KaxVideoColourMaxFALL::ClassInfos,           Y("Video: maximum frame light"),        Y("Maximum frame-average light level in candelas per square meter (cd/m²)."));

  ELE3("chromaticity-coordinates-red-x",   KaxVideoRChromaX::ClassInfos,                Y("Video: chromacity red X"),           Y("Red X chromacity coordinate as defined by CIE 1931."));
  ELE3("chromaticity-coordinates-red-y",   KaxVideoRChromaY::ClassInfos,                Y("Video: chromacity red Y"),           Y("Red Y chromacity coordinate as defined by CIE 1931."));
  ELE3("chromaticity-coordinates-green-x", KaxVideoGChromaX::ClassInfos,                Y("Video: chromacity green X"),         Y("Green X chromacity coordinate as defined by CIE 1931."));
  ELE3("chromaticity-coordinates-green-y", KaxVideoGChromaY::ClassInfos,                Y("Video: chromacity green Y"),         Y("Green Y chromacity coordinate as defined by CIE 1931."));
  ELE3("chromaticity-coordinates-blue-x",  KaxVideoBChromaX::ClassInfos,                Y("Video: chromacity blue X"),          Y("Blue X chromacity coordinate as defined by CIE 1931."));
  ELE3("chromaticity-coordinates-blue-y",  KaxVideoBChromaY::ClassInfos,                Y("Video: chromacity blue Y"),          Y("Blue Y chromacity coordinate as defined by CIE 1931."));
  ELE3("white-coordinates-x",              KaxVideoWhitePointChromaX::ClassInfos,       Y("Video: white point X"),              Y("White colour chromaticity coordinate X as defined by CIE 1931."));
  ELE3("white-coordinates-y",              KaxVideoWhitePointChromaY::ClassInfos,       Y("Video: white point Y"),              Y("White colour chromaticity coordinate Y as defined by CIE 1931."));
  ELE3("max-luminance",                    KaxVideoLuminanceMax::ClassInfos,            Y("Video: maximum luminance"),          Y("Maximum luminance in candelas per square meter (cd/m²)."));
  ELE3("min-luminance",                    KaxVideoLuminanceMin::ClassInfos,            Y("Video: minimum luminance"),          Y("Minimum luminance in candelas per square meter (cd/m²)."));

  sub_sub_master_callbacks = &KaxVideoProjection::ClassInfos;

  ELE2("projection-type",       KaxVideoProjectionType::ClassInfos,      Y("Video: projection type"),             Y("Describes the projection used for this video track (0 – 3)."));
  ELE2("projection-private",    KaxVideoProjectionPrivate::ClassInfos,   Y("Video: projection-specific data"),    Y("Private data that only applies to a specific projection."));
  ELE2("projection-pose-yaw",   KaxVideoProjectionPoseYaw::ClassInfos,   Y("Video: projection's yaw rotation"),   Y("Specifies a yaw rotation to the projection."));
  ELE2("projection-pose-pitch", KaxVideoProjectionPosePitch::ClassInfos, Y("Video: projection's pitch rotation"), Y("Specifies a pitch rotation to the projection."));
  ELE2("projection-pose-roll",  KaxVideoProjectionPoseRoll::ClassInfos,  Y("Video: projection's roll rotation"),  Y("Specifies a roll rotation to the projection."));

  sub_master_callbacks = &KaxTrackAudio::ClassInfos;

  ELE("sampling-frequency",        KaxAudioSamplingFreq::ClassInfos,       Y("Audio sampling frequency"),        Y("Sampling frequency in Hz."));
  ELE("output-sampling-frequency", KaxAudioOutputSamplingFreq::ClassInfos, Y("Audio output sampling frequency"), Y("Real output sampling frequency in Hz."));
  ELE("channels",                  KaxAudioChannels::ClassInfos,           Y("Audio channels"),                  Y("Numbers of channels in the track."));
  ELE("bit-depth",                 KaxAudioBitDepth::ClassInfos,           Y("Audio bit depth"),                 Y("Bits per sample, mostly used for PCM."));
}

std::vector<property_element_c> &
property_element_c::get_table_for(const EbmlCallbacks &master_callbacks,
                                  const EbmlCallbacks *sub_master_callbacks,
                                  bool full_table) {
  if (s_properties.empty())
    init_tables();

  std::map<uint32_t, std::vector<property_element_c> >::iterator src_map_it = s_properties.find(master_callbacks.GlobalId.Value);
  if (s_properties.end() == src_map_it)
    mxerror(boost::format("property_element_c::get_table_for(): programming error: no table found for EBML ID %|1$08x|\n") % master_callbacks.GlobalId.Value);

  if (full_table)
    return src_map_it->second;

  uint32_t element_id = !sub_master_callbacks ? master_callbacks.GlobalId.Value : sub_master_callbacks->GlobalId.Value;
  std::map<uint32_t, std::vector<property_element_c> >::iterator composed_map_it = s_composed_properties.find(element_id);
  if (s_composed_properties.end() != composed_map_it)
    return composed_map_it->second;

  s_composed_properties[element_id]      = std::vector<property_element_c>();
  std::vector<property_element_c> &table = s_composed_properties[element_id];

  for (auto &property : src_map_it->second)
    if (!property.m_sub_master_callbacks || (sub_master_callbacks && (sub_master_callbacks->GlobalId == property.m_sub_master_callbacks->GlobalId)))
      table.push_back(property);

  return table;
}
