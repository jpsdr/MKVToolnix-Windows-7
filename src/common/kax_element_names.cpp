/*
   mkvinfo -- utility for gathering information about Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <ebml/EbmlHead.h>
#include <ebml/EbmlStream.h>
#if LIBEBML_VERSION < 0x020000
# include <ebml/EbmlSubHead.h>
#endif
#include <ebml/EbmlVoid.h>
#include <ebml/EbmlCrc32.h>

#include <matroska/KaxBlock.h>
#include <matroska/KaxBlockData.h>
#include <matroska/KaxCluster.h>
#include <matroska/KaxCues.h>
#include <matroska/KaxCuesData.h>
#if LIBMATROSKA_VERSION < 0x020000
# include <matroska/KaxInfoData.h>
#endif
#include <matroska/KaxSeekHead.h>
#include <matroska/KaxSegment.h>
#include <matroska/KaxTracks.h>
#include <matroska/KaxVersion.h>

#include "common/ebml.h"
#include "common/kax_element_names.h"

namespace mtx {

std::unordered_map<uint32_t, std::string> kax_element_names_c::ms_names;

std::string
kax_element_names_c::get(uint32_t id) {
  init();

  auto itr = ms_names.find(id);
  if (itr != ms_names.end())
    return itr->second;
  return {};
}

std::string
kax_element_names_c::get(libebml::EbmlId const &id) {
  return get(id.GetValue());
}

std::string
kax_element_names_c::get(libebml::EbmlElement const &elt) {
  return get(get_ebml_id(elt).GetValue());
}

void
kax_element_names_c::reset() {
  ms_names.clear();
}

void
kax_element_names_c::init() {
  if (!ms_names.empty())
    return;

  auto add = [](libebml::EbmlId const &id, char const *description) {
    ms_names.insert({ id.GetValue(), description });
  };

  add(EBML_ID(libebml::EbmlHead),                            Y("EBML head"));
  add(EBML_ID(libebml::EVersion),                            Y("EBML version"));
  add(EBML_ID(libebml::EReadVersion),                        Y("EBML read version"));
  add(EBML_ID(libebml::EMaxIdLength),                        Y("Maximum EBML ID length"));
  add(EBML_ID(libebml::EMaxSizeLength),                      Y("Maximum EBML size length"));
  add(EBML_ID(libebml::EDocType),                            Y("Document type"));
  add(EBML_ID(libebml::EDocTypeVersion),                     Y("Document type version"));
  add(EBML_ID(libebml::EDocTypeReadVersion),                 Y("Document type read version"));
  add(EBML_ID(libebml::EbmlVoid),                            Y("EBML void"));
  add(EBML_ID(libebml::EbmlCrc32),                           Y("EBML CRC-32"));

  add(EBML_ID(libmatroska::KaxSegment),                      Y("Segment"));
  add(EBML_ID(libmatroska::KaxInfo),                         Y("Segment information"));
  add(EBML_ID(kax_timestamp_scale_c),                        Y("Timestamp scale"));
  add(EBML_ID(libmatroska::KaxDuration),                     Y("Duration"));
  add(EBML_ID(libmatroska::KaxMuxingApp),                    Y("Multiplexing application"));
  add(EBML_ID(libmatroska::KaxWritingApp),                   Y("Writing application"));
  add(EBML_ID(libmatroska::KaxDateUTC),                      Y("Date"));
  add(EBML_ID(libmatroska::KaxSegmentUID),                   Y("Segment UID"));
  add(EBML_ID(libmatroska::KaxSegmentFamily),                Y("Family UID"));
  add(EBML_ID(libmatroska::KaxPrevUID),                      Y("Previous segment UID"));
  add(EBML_ID(libmatroska::KaxPrevFilename),                 Y("Previous filename"));
  add(EBML_ID(libmatroska::KaxNextUID),                      Y("Next segment UID"));
  add(EBML_ID(libmatroska::KaxNextFilename),                 Y("Next filename"));
  add(EBML_ID(libmatroska::KaxSegmentFilename),              Y("Segment filename"));
  add(EBML_ID(libmatroska::KaxTitle),                        Y("Title"));

  add(EBML_ID(libmatroska::KaxChapterTranslate),             Y("Chapter translate"));
  add(EBML_ID(libmatroska::KaxChapterTranslateEditionUID),   Y("Chapter translate edition UID"));
  add(EBML_ID(libmatroska::KaxChapterTranslateCodec),        Y("Chapter translate codec"));
  add(EBML_ID(libmatroska::KaxChapterTranslateID),           Y("Chapter translate ID"));

  add(EBML_ID(libmatroska::KaxTrackAudio),                   Y("Audio track"));
  add(EBML_ID(libmatroska::KaxAudioSamplingFreq),            Y("Sampling frequency"));
  add(EBML_ID(libmatroska::KaxAudioOutputSamplingFreq),      Y("Output sampling frequency"));
  add(EBML_ID(libmatroska::KaxAudioChannels),                Y("Channels"));
  add(EBML_ID(libmatroska::KaxAudioPosition),                Y("Channel positions"));
  add(EBML_ID(libmatroska::KaxAudioBitDepth),                Y("Bit depth"));
  add(EBML_ID(libmatroska::KaxEmphasis),                     Y("Emphasis"));

  add(EBML_ID(libmatroska::KaxVideoColourMasterMeta),        Y("Video color mastering metadata"));
  add(EBML_ID(libmatroska::KaxVideoRChromaX),                Y("Red color coordinate x"));
  add(EBML_ID(libmatroska::KaxVideoRChromaY),                Y("Red color coordinate y"));
  add(EBML_ID(libmatroska::KaxVideoGChromaX),                Y("Green color coordinate x"));
  add(EBML_ID(libmatroska::KaxVideoGChromaY),                Y("Green color coordinate y"));
  add(EBML_ID(libmatroska::KaxVideoBChromaX),                Y("Blue color coordinate x"));
  add(EBML_ID(libmatroska::KaxVideoBChromaY),                Y("Blue color coordinate y"));
  add(EBML_ID(libmatroska::KaxVideoWhitePointChromaX),       Y("White color coordinate x"));
  add(EBML_ID(libmatroska::KaxVideoWhitePointChromaY),       Y("White color coordinate y"));
  add(EBML_ID(libmatroska::KaxVideoLuminanceMax),            Y("Maximum luminance"));
  add(EBML_ID(libmatroska::KaxVideoLuminanceMin),            Y("Minimum luminance"));

  add(EBML_ID(libmatroska::KaxVideoColour),                  Y("Video color information"));
  add(EBML_ID(libmatroska::KaxVideoColourMatrix),            Y("Color matrix coefficients"));
  add(EBML_ID(libmatroska::KaxVideoBitsPerChannel),          Y("Bits per channel"));
  add(EBML_ID(libmatroska::KaxVideoChromaSubsampHorz),       Y("Horizontal chroma subsample"));
  add(EBML_ID(libmatroska::KaxVideoChromaSubsampVert),       Y("Vertical chroma subsample"));
  add(EBML_ID(libmatroska::KaxVideoCbSubsampHorz),           Y("Horizontal Cb subsample"));
  add(EBML_ID(libmatroska::KaxVideoCbSubsampVert),           Y("Vertical Cb subsample"));
  add(EBML_ID(libmatroska::KaxVideoChromaSitHorz),           Y("Horizontal chroma siting"));
  add(EBML_ID(libmatroska::KaxVideoChromaSitVert),           Y("Vertical chroma siting"));
  add(EBML_ID(libmatroska::KaxVideoColourRange),             Y("Color range"));
  add(EBML_ID(libmatroska::KaxVideoColourTransferCharacter), Y("Color transfer"));
  add(EBML_ID(libmatroska::KaxVideoColourPrimaries),         Y("Color primaries"));
  add(EBML_ID(libmatroska::KaxVideoColourMaxCLL),            Y("Maximum content light"));
  add(EBML_ID(libmatroska::KaxVideoColourMaxFALL),           Y("Maximum frame light"));

  add(EBML_ID(libmatroska::KaxVideoProjection),              Y("Video projection"));
  add(EBML_ID(libmatroska::KaxVideoProjectionType),          Y("Projection type"));
  add(EBML_ID(libmatroska::KaxVideoProjectionPrivate),       Y("Projection's private data"));
  add(EBML_ID(libmatroska::KaxVideoProjectionPoseYaw),       Y("Projection's yaw rotation"));
  add(EBML_ID(libmatroska::KaxVideoProjectionPosePitch),     Y("Projection's pitch rotation"));
  add(EBML_ID(libmatroska::KaxVideoProjectionPoseRoll),      Y("Projection's roll rotation"));

  add(EBML_ID(libmatroska::KaxTrackVideo),                   Y("Video track"));
  add(EBML_ID(libmatroska::KaxVideoPixelWidth),              Y("Pixel width"));
  add(EBML_ID(libmatroska::KaxVideoPixelHeight),             Y("Pixel height"));
  add(EBML_ID(libmatroska::KaxVideoDisplayWidth),            Y("Display width"));
  add(EBML_ID(libmatroska::KaxVideoDisplayHeight),           Y("Display height"));
  add(EBML_ID(libmatroska::KaxVideoPixelCropLeft),           Y("Pixel crop left"));
  add(EBML_ID(libmatroska::KaxVideoPixelCropTop),            Y("Pixel crop top"));
  add(EBML_ID(libmatroska::KaxVideoPixelCropRight),          Y("Pixel crop right"));
  add(EBML_ID(libmatroska::KaxVideoPixelCropBottom),         Y("Pixel crop bottom"));
  add(EBML_ID(libmatroska::KaxVideoDisplayUnit),             Y("Display unit"));
  add(EBML_ID(libmatroska::KaxVideoGamma),                   Y("Gamma"));
  add(EBML_ID(libmatroska::KaxVideoFlagInterlaced),          Y("Interlaced"));
  add(EBML_ID(libmatroska::KaxVideoFieldOrder),              Y("Field order"));
  add(EBML_ID(libmatroska::KaxVideoStereoMode),              Y("Video stereo mode"));
  add(EBML_ID(libmatroska::KaxVideoAspectRatio),             Y("Aspect ratio type"));
  add(EBML_ID(libmatroska::KaxVideoColourSpace),             Y("Color space"));
  add(EBML_ID(libmatroska::KaxVideoFrameRate),               Y("Frame rate"));
  add(EBML_ID(libmatroska::KaxOldStereoMode),                Y("Stereo mode (deprecated element)"));
  add(EBML_ID(libmatroska::KaxVideoAlphaMode),               Y("Video alpha mode"));

  add(EBML_ID(libmatroska::KaxContentEncodings),             Y("Content encodings"));
  add(EBML_ID(libmatroska::KaxContentEncoding),              Y("Content encoding"));
  add(EBML_ID(libmatroska::KaxContentEncodingOrder),         Y("Order"));
  add(EBML_ID(libmatroska::KaxContentEncodingScope),         Y("Scope"));
  add(EBML_ID(libmatroska::KaxContentEncodingType),          Y("Type"));
  add(EBML_ID(libmatroska::KaxContentCompression),           Y("Content compression"));
  add(EBML_ID(libmatroska::KaxContentCompAlgo),              Y("Algorithm"));
  add(EBML_ID(libmatroska::KaxContentCompSettings),          Y("Settings"));
  add(EBML_ID(libmatroska::KaxContentEncryption),            Y("Content encryption"));
  add(EBML_ID(libmatroska::KaxContentEncAlgo),               Y("Encryption algorithm"));
  add(EBML_ID(libmatroska::KaxContentEncKeyID),              Y("Encryption key ID"));
  add(EBML_ID(libmatroska::KaxContentSigAlgo),               Y("Signature algorithm"));
  add(EBML_ID(libmatroska::KaxContentSigHashAlgo),           Y("Signature hash algorithm"));
  add(EBML_ID(libmatroska::KaxContentSigKeyID),              Y("Signature key ID"));
  add(EBML_ID(libmatroska::KaxContentSignature),             Y("Signature"));

  add(EBML_ID(libmatroska::KaxBlockAdditionMapping),         Y("Block addition mapping"));
  add(EBML_ID(libmatroska::KaxBlockAddIDExtraData),          Y("Block addition ID extra data"));
  add(EBML_ID(libmatroska::KaxBlockAddIDName),               Y("Block addition ID name"));
  add(EBML_ID(libmatroska::KaxBlockAddIDType),               Y("Block addition ID type"));
  add(EBML_ID(libmatroska::KaxBlockAddIDValue),              Y("Block addition ID value"));

  add(EBML_ID(libmatroska::KaxTracks),                       Y("Tracks"));
  add(EBML_ID(libmatroska::KaxTrackEntry),                   Y("Track"));
  add(EBML_ID(libmatroska::KaxTrackNumber),                  Y("Track number"));
  add(EBML_ID(libmatroska::KaxTrackUID),                     Y("Track UID"));
  add(EBML_ID(libmatroska::KaxTrackType),                    Y("Track type"));
  add(EBML_ID(libmatroska::KaxTrackFlagEnabled),             Y("\"Enabled\" flag"));
  add(EBML_ID(libmatroska::KaxTrackName),                    Y("Name"));
  add(EBML_ID(libmatroska::KaxCodecID),                      Y("Codec ID"));
  add(EBML_ID(libmatroska::KaxCodecPrivate),                 Y("Codec's private data"));
  add(EBML_ID(libmatroska::KaxCodecName),                    Y("Codec name"));
  add(EBML_ID(libmatroska::KaxCodecDelay),                   Y("Codec-inherent delay"));
  add(EBML_ID(libmatroska::KaxCodecSettings),                Y("Codec settings"));
  add(EBML_ID(libmatroska::KaxCodecInfoURL),                 Y("Codec info URL"));
  add(EBML_ID(libmatroska::KaxCodecDownloadURL),             Y("Codec download URL"));
  add(EBML_ID(libmatroska::KaxCodecDecodeAll),               Y("Codec decode all"));
  add(EBML_ID(libmatroska::KaxTrackOverlay),                 Y("Track overlay"));
  add(EBML_ID(libmatroska::KaxTrackMinCache),                Y("Minimum cache"));
  add(EBML_ID(libmatroska::KaxTrackMaxCache),                Y("Maximum cache"));
  add(EBML_ID(libmatroska::KaxTrackDefaultDuration),         Y("Default duration"));
  add(EBML_ID(libmatroska::KaxTrackFlagLacing),              Y("\"Lacing\" flag"));
  add(EBML_ID(libmatroska::KaxTrackFlagDefault),             Y("\"Default track\" flag"));
  add(EBML_ID(libmatroska::KaxTrackFlagForced),              Y("\"Forced display\" flag"));
  add(EBML_ID(libmatroska::KaxTrackLanguage),                Y("Language"));
  add(EBML_ID(libmatroska::KaxLanguageIETF),                 Y("Language (IETF BCP 47)"));
  add(EBML_ID(kax_track_timestamp_scale_c),                  Y("Timestamp scale"));
  add(EBML_ID(libmatroska::KaxMaxBlockAdditionID),           Y("Maximum block additional ID"));
  add(EBML_ID(libmatroska::KaxContentEncodings),             Y("Content encodings"));
  add(EBML_ID(libmatroska::KaxSeekPreRoll),                  Y("Seek pre-roll"));
  add(EBML_ID(libmatroska::KaxFlagHearingImpaired),          Y("\"Hearing impaired\" flag"));
  add(EBML_ID(libmatroska::KaxFlagVisualImpaired),           Y("\"Visual impaired\" flag"));
  add(EBML_ID(libmatroska::KaxFlagTextDescriptions),         Y("\"Text descriptions\" flag"));
  add(EBML_ID(libmatroska::KaxFlagOriginal),                 Y("\"Original language\" flag"));
  add(EBML_ID(libmatroska::KaxFlagCommentary),               Y("\"Commentary\" flag"));

  add(EBML_ID(libmatroska::KaxSeekHead),                     Y("Seek head"));
  add(EBML_ID(libmatroska::KaxSeek),                         Y("Seek entry"));
  add(EBML_ID(libmatroska::KaxSeekID),                       Y("Seek ID"));
  add(EBML_ID(libmatroska::KaxSeekPosition),                 Y("Seek position"));

  add(EBML_ID(libmatroska::KaxCues),                         Y("Cues"));
  add(EBML_ID(libmatroska::KaxCuePoint),                     Y("Cue point"));
  add(EBML_ID(libmatroska::KaxCueTime),                      Y("Cue time"));
  add(EBML_ID(libmatroska::KaxCueTrackPositions),            Y("Cue track positions"));
  add(EBML_ID(libmatroska::KaxCueTrack),                     Y("Cue track"));
  add(EBML_ID(libmatroska::KaxCueClusterPosition),           Y("Cue cluster position"));
  add(EBML_ID(libmatroska::KaxCueRelativePosition),          Y("Cue relative position"));
  add(EBML_ID(libmatroska::KaxCueDuration),                  Y("Cue duration"));
  add(EBML_ID(libmatroska::KaxCueBlockNumber),               Y("Cue block number"));
  add(EBML_ID(libmatroska::KaxCueCodecState),                Y("Cue codec state"));
  add(EBML_ID(libmatroska::KaxCueReference),                 Y("Cue reference"));
  add(EBML_ID(libmatroska::KaxCueRefTime),                   Y("Cue ref time"));
  add(EBML_ID(libmatroska::KaxCueRefCluster),                Y("Cue ref cluster"));
  add(EBML_ID(libmatroska::KaxCueRefNumber),                 Y("Cue ref number"));
  add(EBML_ID(libmatroska::KaxCueRefCodecState),             Y("Cue ref codec state"));

  add(EBML_ID(libmatroska::KaxAttachments),                  Y("Attachments"));
  add(EBML_ID(libmatroska::KaxAttached),                     Y("Attached"));
  add(EBML_ID(libmatroska::KaxFileDescription),              Y("File description"));
  add(EBML_ID(libmatroska::KaxFileName),                     Y("File name"));
  add(EBML_ID(libmatroska::KaxMimeType),                     Y("MIME type"));
  add(EBML_ID(libmatroska::KaxFileData),                     Y("File data"));
  add(EBML_ID(libmatroska::KaxFileUID),                      Y("File UID"));

  add(EBML_ID(libmatroska::KaxClusterSilentTracks),          Y("Silent tracks"));
  add(EBML_ID(libmatroska::KaxClusterSilentTrackNumber),     Y("Silent track number"));

  add(EBML_ID(libmatroska::KaxBlockGroup),                   Y("Block group"));
  add(EBML_ID(libmatroska::KaxBlock),                        Y("Block"));
  add(EBML_ID(libmatroska::KaxBlockDuration),                Y("Block duration"));
  add(EBML_ID(libmatroska::KaxReferenceBlock),               Y("Reference block"));
  add(EBML_ID(libmatroska::KaxReferencePriority),            Y("Reference priority"));
  add(EBML_ID(libmatroska::KaxBlockVirtual),                 Y("Block virtual"));
  add(EBML_ID(libmatroska::KaxReferenceVirtual),             Y("Reference virtual"));
  add(EBML_ID(libmatroska::KaxCodecState),                   Y("Codec state"));
  add(EBML_ID(libmatroska::KaxBlockAddID),                   Y("Block additional ID"));
  add(EBML_ID(libmatroska::KaxBlockAdditional),              Y("Block additional"));
  add(EBML_ID(libmatroska::KaxSliceLaceNumber),              Y("Lace number"));
  add(EBML_ID(libmatroska::KaxSliceFrameNumber),             Y("Frame number"));
  add(EBML_ID(libmatroska::KaxSliceDelay),                   Y("Delay"));
  add(EBML_ID(libmatroska::KaxSliceDuration),                Y("Duration"));
  add(EBML_ID(libmatroska::KaxSliceBlockAddID),              Y("Block additional ID"));
  add(EBML_ID(libmatroska::KaxDiscardPadding),               Y("Discard padding"));
  add(EBML_ID(libmatroska::KaxBlockAdditions),               Y("Additions"));
  add(EBML_ID(libmatroska::KaxBlockMore),                    Y("More"));
  add(EBML_ID(libmatroska::KaxSlices),                       Y("Slices"));
  add(EBML_ID(libmatroska::KaxTimeSlice),                    Y("Time slice"));
  add(EBML_ID(libmatroska::KaxSimpleBlock),                  Y("Simple block"));

  add(EBML_ID(libmatroska::KaxCluster),                      Y("Cluster"));
  add(EBML_ID(kax_cluster_timestamp_c),                      Y("Cluster timestamp"));
  add(EBML_ID(libmatroska::KaxClusterPosition),              Y("Cluster position"));
  add(EBML_ID(libmatroska::KaxClusterPrevSize),              Y("Cluster previous size"));

  add(EBML_ID(libmatroska::KaxChapters),                     Y("Chapters"));
  add(EBML_ID(libmatroska::KaxEditionEntry),                 Y("Edition entry"));
  add(EBML_ID(libmatroska::KaxEditionUID),                   Y("Edition UID"));
  add(EBML_ID(libmatroska::KaxEditionFlagHidden),            Y("Edition flag hidden"));
  add(EBML_ID(libmatroska::KaxEditionFlagDefault),           Y("Edition flag default"));
  add(EBML_ID(libmatroska::KaxEditionFlagOrdered),           Y("Edition flag ordered"));
  add(EBML_ID(libmatroska::KaxEditionDisplay),               Y("Edition display"));
  add(EBML_ID(libmatroska::KaxEditionString),                Y("Edition string"));
  add(EBML_ID(libmatroska::KaxEditionLanguageIETF),          Y("Edition language (IETF BCP 47)"));
  add(EBML_ID(libmatroska::KaxChapterAtom),                  Y("Chapter atom"));
  add(EBML_ID(libmatroska::KaxChapterUID),                   Y("Chapter UID"));
  add(EBML_ID(libmatroska::KaxChapterStringUID),             Y("Chapter string UID"));
  add(EBML_ID(libmatroska::KaxChapterTimeStart),             Y("Chapter time start"));
  add(EBML_ID(libmatroska::KaxChapterTimeEnd),               Y("Chapter time end"));
  add(EBML_ID(libmatroska::KaxChapterFlagHidden),            Y("Chapter flag hidden"));
  add(EBML_ID(libmatroska::KaxChapterFlagEnabled),           Y("Chapter flag enabled"));
  add(EBML_ID(libmatroska::KaxChapterSegmentUID),            Y("Chapter segment UID"));
  add(EBML_ID(libmatroska::KaxChapterSegmentEditionUID),     Y("Chapter segment edition UID"));
  add(EBML_ID(libmatroska::KaxChapterPhysicalEquiv),         Y("Chapter physical equivalent"));
  add(EBML_ID(libmatroska::KaxChapterTrack),                 Y("Chapter track"));
  add(EBML_ID(libmatroska::KaxChapterTrackNumber),           Y("Chapter track number"));
  add(EBML_ID(libmatroska::KaxChapterDisplay),               Y("Chapter display"));
  add(EBML_ID(libmatroska::KaxChapterString),                Y("Chapter string"));
  add(EBML_ID(libmatroska::KaxChapterLanguage),              Y("Chapter language"));
  add(EBML_ID(libmatroska::KaxChapLanguageIETF),             Y("Chapter language (IETF BCP 47)"));
  add(EBML_ID(libmatroska::KaxChapterCountry),               Y("Chapter country"));
  add(EBML_ID(libmatroska::KaxChapterProcess),               Y("Chapter process"));
  add(EBML_ID(libmatroska::KaxChapterProcessCodecID),        Y("Chapter process codec ID"));
  add(EBML_ID(libmatroska::KaxChapterProcessPrivate),        Y("Chapter process private"));
  add(EBML_ID(libmatroska::KaxChapterProcessCommand),        Y("Chapter process command"));
  add(EBML_ID(libmatroska::KaxChapterProcessTime),           Y("Chapter process time"));
  add(EBML_ID(libmatroska::KaxChapterProcessData),           Y("Chapter process data"));
  add(EBML_ID(libmatroska::KaxChapterSkipType),              Y("Chapter skip type"));

  add(EBML_ID(libmatroska::KaxTags),                         Y("Tags"));
  add(EBML_ID(libmatroska::KaxTag),                          Y("Tag"));
  add(EBML_ID(libmatroska::KaxTagTargets),                   Y("Targets"));
  add(EBML_ID(libmatroska::KaxTagTrackUID),                  Y("Track UID"));
  add(EBML_ID(libmatroska::KaxTagEditionUID),                Y("Edition UID"));
  add(EBML_ID(libmatroska::KaxTagChapterUID),                Y("Chapter UID"));
  add(EBML_ID(libmatroska::KaxTagAttachmentUID),             Y("Attachment UID"));
  add(EBML_ID(libmatroska::KaxTagTargetType),                Y("Target type"));
  add(EBML_ID(libmatroska::KaxTagTargetTypeValue),           Y("Target type value"));
  add(EBML_ID(libmatroska::KaxTagSimple),                    Y("Simple"));
  add(EBML_ID(libmatroska::KaxTagName),                      Y("Name"));
  add(EBML_ID(libmatroska::KaxTagString),                    Y("String"));
  add(EBML_ID(libmatroska::KaxTagBinary),                    Y("Binary"));
  add(EBML_ID(libmatroska::KaxTagLangue),                    Y("Tag language"));
  add(EBML_ID(libmatroska::KaxTagLanguageIETF),              Y("Tag language (IETF BCP 47)"));
  add(EBML_ID(libmatroska::KaxTagDefault),                   Y("Default language"));
  add(EBML_ID(libmatroska::KaxTagDefaultBogus),              Y("Default language (bogus element with invalid ID)"));
}

}
