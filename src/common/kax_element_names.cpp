/*
   mkvinfo -- utility for gathering information about Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <ebml/EbmlHead.h>
#include <ebml/EbmlSubHead.h>
#include <ebml/EbmlStream.h>
#include <ebml/EbmlVoid.h>
#include <ebml/EbmlCrc32.h>

#include <matroska/KaxAttached.h>
#include <matroska/KaxAttachments.h>
#include <matroska/KaxBlock.h>
#include <matroska/KaxBlockData.h>
#include <matroska/KaxChapters.h>
#include <matroska/KaxCluster.h>
#include <matroska/KaxClusterData.h>
#include <matroska/KaxContentEncoding.h>
#include <matroska/KaxCues.h>
#include <matroska/KaxCuesData.h>
#include <matroska/KaxInfo.h>
#include <matroska/KaxInfoData.h>
#include <matroska/KaxSeekHead.h>
#include <matroska/KaxSegment.h>
#include <matroska/KaxTags.h>
#include <matroska/KaxTag.h>
#include <matroska/KaxTracks.h>
#include <matroska/KaxTrackEntryData.h>
#include <matroska/KaxTrackAudio.h>
#include <matroska/KaxTrackVideo.h>
#include <matroska/KaxVersion.h>

#include "common/kax_element_names.h"

using namespace libmatroska;

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

void
kax_element_names_c::reset() {
  ms_names.clear();
}

#define ADD(Class, Name) ms_names.insert({ Class::ClassInfos.GlobalId.GetValue(), std::string{Name} })

void
kax_element_names_c::init() {
  if (!ms_names.empty())
    return;

  ADD(EbmlHead,                        Y("EBML head"));
  ADD(EVersion,                        Y("EBML version"));
  ADD(EReadVersion,                    Y("EBML read version"));
  ADD(EMaxIdLength,                    Y("Maximum EBML ID length"));
  ADD(EMaxSizeLength,                  Y("Maximum EBML size length"));
  ADD(EDocType,                        Y("Document type"));
  ADD(EDocTypeVersion,                 Y("Document type version"));
  ADD(EDocTypeReadVersion,             Y("Document type read version"));
  ADD(EbmlVoid,                        Y("EBML void"));
  ADD(EbmlCrc32,                       Y("EBML CRC-32"));

  ADD(KaxSegment,                      Y("Segment"));
  ADD(KaxInfo,                         Y("Segment information"));
  ADD(KaxTimecodeScale,                Y("Timestamp scale"));
  ADD(KaxDuration,                     Y("Duration"));
  ADD(KaxMuxingApp,                    Y("Multiplexing application"));
  ADD(KaxWritingApp,                   Y("Writing application"));
  ADD(KaxDateUTC,                      Y("Date"));
  ADD(KaxSegmentUID,                   Y("Segment UID"));
  ADD(KaxSegmentFamily,                Y("Family UID"));
  ADD(KaxPrevUID,                      Y("Previous segment UID"));
  ADD(KaxPrevFilename,                 Y("Previous filename"));
  ADD(KaxNextUID,                      Y("Next segment UID"));
  ADD(KaxNextFilename,                 Y("Next filename"));
  ADD(KaxSegmentFilename,              Y("Segment filename"));
  ADD(KaxTitle,                        Y("Title"));

  ADD(KaxChapterTranslate,             Y("Chapter translate"));
  ADD(KaxChapterTranslateEditionUID,   Y("Chapter translate edition UID"));
  ADD(KaxChapterTranslateCodec,        Y("Chapter translate codec"));
  ADD(KaxChapterTranslateID,           Y("Chapter translate ID"));

  ADD(KaxTrackAudio,                   Y("Audio track"));
  ADD(KaxAudioSamplingFreq,            Y("Sampling frequency"));
  ADD(KaxAudioOutputSamplingFreq,      Y("Output sampling frequency"));
  ADD(KaxAudioChannels,                Y("Channels"));
  ADD(KaxAudioPosition,                Y("Channel positions"));
  ADD(KaxAudioBitDepth,                Y("Bit depth"));

  ADD(KaxVideoColourMasterMeta,        Y("Video colour mastering metadata"));
  ADD(KaxVideoRChromaX,                Y("Red colour coordinate x"));
  ADD(KaxVideoRChromaY,                Y("Red colour coordinate y"));
  ADD(KaxVideoGChromaX,                Y("Green colour coordinate x"));
  ADD(KaxVideoGChromaY,                Y("Green colour coordinate y"));
  ADD(KaxVideoBChromaX,                Y("Blue colour coordinate x"));
  ADD(KaxVideoBChromaY,                Y("Blue colour coordinate y"));
  ADD(KaxVideoWhitePointChromaX,       Y("White colour coordinate x"));
  ADD(KaxVideoWhitePointChromaY,       Y("White colour coordinate y"));
  ADD(KaxVideoLuminanceMax,            Y("Maximum luminance"));
  ADD(KaxVideoLuminanceMin,            Y("Minimum luminance"));

  ADD(KaxVideoColour,                  Y("Video colour information"));
  ADD(KaxVideoColourMatrix,            Y("Colour matrix coefficients"));
  ADD(KaxVideoBitsPerChannel,          Y("Bits per channel"));
  ADD(KaxVideoChromaSubsampHorz,       Y("Horizontal chroma subsample"));
  ADD(KaxVideoChromaSubsampVert,       Y("Vertical chroma subsample"));
  ADD(KaxVideoCbSubsampHorz,           Y("Horizontal Cb subsample"));
  ADD(KaxVideoCbSubsampVert,           Y("Vertical Cb subsample"));
  ADD(KaxVideoChromaSitHorz,           Y("Horizontal chroma siting"));
  ADD(KaxVideoChromaSitVert,           Y("Vertical chroma siting"));
  ADD(KaxVideoColourRange,             Y("Colour range"));
  ADD(KaxVideoColourTransferCharacter, Y("Colour transfer"));
  ADD(KaxVideoColourPrimaries,         Y("Colour primaries"));
  ADD(KaxVideoColourMaxCLL,            Y("Maximum content light"));
  ADD(KaxVideoColourMaxFALL,           Y("Maximum frame light"));

  ADD(KaxVideoProjection,              Y("Video projection"));
  ADD(KaxVideoProjectionType,          Y("Projection type"));
  ADD(KaxVideoProjectionPrivate,       Y("Projection's private data"));
  ADD(KaxVideoProjectionPoseYaw,       Y("Projection's yaw rotation"));
  ADD(KaxVideoProjectionPosePitch,     Y("Projection's pitch rotation"));
  ADD(KaxVideoProjectionPoseRoll,      Y("Projection's roll rotation"));

  ADD(KaxTrackVideo,                   Y("Video track"));
  ADD(KaxVideoPixelWidth,              Y("Pixel width"));
  ADD(KaxVideoPixelHeight,             Y("Pixel height"));
  ADD(KaxVideoDisplayWidth,            Y("Display width"));
  ADD(KaxVideoDisplayHeight,           Y("Display height"));
  ADD(KaxVideoPixelCropLeft,           Y("Pixel crop left"));
  ADD(KaxVideoPixelCropTop,            Y("Pixel crop top"));
  ADD(KaxVideoPixelCropRight,          Y("Pixel crop right"));
  ADD(KaxVideoPixelCropBottom,         Y("Pixel crop bottom"));
  ADD(KaxVideoDisplayUnit,             Y("Display unit"));
  ADD(KaxVideoGamma,                   Y("Gamma"));
  ADD(KaxVideoFlagInterlaced,          Y("Interlaced"));
  ADD(KaxVideoFieldOrder,              Y("Field order"));
  ADD(KaxVideoStereoMode,              Y("Stereo mode"));
  ADD(KaxVideoAspectRatio,             Y("Aspect ratio type"));
  ADD(KaxVideoColourSpace,             Y("Colour space"));
  ADD(KaxVideoFrameRate,               Y("Frame rate"));

  ADD(KaxContentEncodings,             Y("Content encodings"));
  ADD(KaxContentEncoding,              Y("Content encoding"));
  ADD(KaxContentEncodingOrder,         Y("Order"));
  ADD(KaxContentEncodingScope,         Y("Scope"));
  ADD(KaxContentEncodingType,          Y("Type"));
  ADD(KaxContentCompression,           Y("Content compression"));
  ADD(KaxContentCompAlgo,              Y("Algorithm"));
  ADD(KaxContentCompSettings,          Y("Settings"));
  ADD(KaxContentEncryption,            Y("Content encryption"));
  ADD(KaxContentEncAlgo,               Y("Encryption algorithm"));
  ADD(KaxContentEncKeyID,              Y("Encryption key ID"));
  ADD(KaxContentSigAlgo,               Y("Signature algorithm"));
  ADD(KaxContentSigHashAlgo,           Y("Signature hash algorithm"));
  ADD(KaxContentSigKeyID,              Y("Signature key ID"));
  ADD(KaxContentSignature,             Y("Signature"));

  ADD(KaxBlockAdditionMapping,         Y("Block addition mapping"));
  ADD(KaxBlockAddIDExtraData,          Y("Block addition ID extra data"));
  ADD(KaxBlockAddIDName,               Y("Block addition ID name"));
  ADD(KaxBlockAddIDType,               Y("Block addition ID type"));
  ADD(KaxBlockAddIDValue,              Y("Block addition ID value"));

  ADD(KaxTracks,                       Y("Tracks"));
  ADD(KaxTrackEntry,                   Y("Track"));
  ADD(KaxTrackNumber,                  Y("Track number"));
  ADD(KaxTrackUID,                     Y("Track UID"));
  ADD(KaxTrackType,                    Y("Track type"));
  ADD(KaxTrackFlagEnabled,             Y("'Enabled' flag"));
  ADD(KaxTrackName,                    Y("Name"));
  ADD(KaxCodecID,                      Y("Codec ID"));
  ADD(KaxCodecPrivate,                 Y("Codec's private data"));
  ADD(KaxCodecName,                    Y("Codec name"));
  ADD(KaxCodecDelay,                   Y("Codec-inherent delay"));
  ADD(KaxCodecSettings,                Y("Codec settings"));
  ADD(KaxCodecInfoURL,                 Y("Codec info URL"));
  ADD(KaxCodecDownloadURL,             Y("Codec download URL"));
  ADD(KaxCodecDecodeAll,               Y("Codec decode all"));
  ADD(KaxTrackOverlay,                 Y("Track overlay"));
  ADD(KaxTrackMinCache,                Y("Minimum cache"));
  ADD(KaxTrackMaxCache,                Y("Maximum cache"));
  ADD(KaxTrackDefaultDuration,         Y("Default duration"));
  ADD(KaxTrackFlagLacing,              Y("'Lacing' flag"));
  ADD(KaxTrackFlagDefault,             Y("'Default track' flag"));
  ADD(KaxTrackFlagForced,              Y("'Forced display' flag"));
  ADD(KaxTrackLanguage,                Y("Language"));
  ADD(KaxLanguageIETF,                 Y("Language (IETF BCP 47)"));
  ADD(KaxTrackTimecodeScale,           Y("Timestamp scale"));
  ADD(KaxMaxBlockAdditionID,           Y("Maximum block additional ID"));
  ADD(KaxContentEncodings,             Y("Content encodings"));
  ADD(KaxSeekPreRoll,                  Y("Seek pre-roll"));
  ADD(KaxFlagHearingImpaired,          Y("\"Hearing impaired\" flag"));
  ADD(KaxFlagVisualImpaired,           Y("\"Visual impaired\" flag"));
  ADD(KaxFlagTextDescriptions,         Y("\"Text descriptions\" flag"));
  ADD(KaxFlagOriginal,                 Y("\"Original language\" flag"));
  ADD(KaxFlagCommentary,               Y("\"Commentary\" flag"));

  ADD(KaxSeekHead,                     Y("Seek head"));
  ADD(KaxSeek,                         Y("Seek entry"));
  ADD(KaxSeekID,                       Y("Seek ID"));
  ADD(KaxSeekPosition,                 Y("Seek position"));

  ADD(KaxCues,                         Y("Cues"));
  ADD(KaxCuePoint,                     Y("Cue point"));
  ADD(KaxCueTime,                      Y("Cue time"));
  ADD(KaxCueTrackPositions,            Y("Cue track positions"));
  ADD(KaxCueTrack,                     Y("Cue track"));
  ADD(KaxCueClusterPosition,           Y("Cue cluster position"));
  ADD(KaxCueRelativePosition,          Y("Cue relative position"));
  ADD(KaxCueDuration,                  Y("Cue duration"));
  ADD(KaxCueBlockNumber,               Y("Cue block number"));
  ADD(KaxCueCodecState,                Y("Cue codec state"));
  ADD(KaxCueReference,                 Y("Cue reference"));
  ADD(KaxCueRefTime,                   Y("Cue ref time"));
  ADD(KaxCueRefCluster,                Y("Cue ref cluster"));
  ADD(KaxCueRefNumber,                 Y("Cue ref number"));
  ADD(KaxCueRefCodecState,             Y("Cue ref codec state"));

  ADD(KaxAttachments,                  Y("Attachments"));
  ADD(KaxAttached,                     Y("Attached"));
  ADD(KaxFileDescription,              Y("File description"));
  ADD(KaxFileName,                     Y("File name"));
  ADD(KaxMimeType,                     Y("MIME type"));
  ADD(KaxFileData,                     Y("File data"));
  ADD(KaxFileUID,                      Y("File UID"));

  ADD(KaxClusterSilentTracks,          Y("Silent tracks"));
  ADD(KaxClusterSilentTrackNumber,     Y("Silent track number"));

  ADD(KaxBlockGroup,                   Y("Block group"));
  ADD(KaxBlock,                        Y("Block"));
  ADD(KaxBlockDuration,                Y("Block duration"));
  ADD(KaxReferenceBlock,               Y("Reference block"));
  ADD(KaxReferencePriority,            Y("Reference priority"));
  ADD(KaxBlockVirtual,                 Y("Block virtual"));
  ADD(KaxReferenceVirtual,             Y("Reference virtual"));
  ADD(KaxCodecState,                   Y("Codec state"));
  ADD(KaxBlockAddID,                   Y("Block additional ID"));
  ADD(KaxBlockAdditional,              Y("Block additional"));
  ADD(KaxSliceLaceNumber,              Y("Lace number"));
  ADD(KaxSliceFrameNumber,             Y("Frame number"));
  ADD(KaxSliceDelay,                   Y("Delay"));
  ADD(KaxSliceDuration,                Y("Duration"));
  ADD(KaxSliceBlockAddID,              Y("Block additional ID"));
  ADD(KaxDiscardPadding,               Y("Discard padding"));
  ADD(KaxBlockAdditions,               Y("Additions"));
  ADD(KaxBlockMore,                    Y("More"));
  ADD(KaxSlices,                       Y("Slices"));
  ADD(KaxTimeSlice,                    Y("Time slice"));
  ADD(KaxSimpleBlock,                  Y("Simple block"));

  ADD(KaxCluster,                      Y("Cluster"));
  ADD(KaxClusterTimecode,              Y("Cluster timestamp"));
  ADD(KaxClusterPosition,              Y("Cluster position"));
  ADD(KaxClusterPrevSize,              Y("Cluster previous size"));

  ADD(KaxChapters,                     Y("Chapters"));
  ADD(KaxEditionEntry,                 Y("Edition entry"));
  ADD(KaxEditionUID,                   Y("Edition UID"));
  ADD(KaxEditionFlagHidden,            Y("Edition flag hidden"));
  ADD(KaxEditionFlagDefault,           Y("Edition flag default"));
  ADD(KaxEditionFlagOrdered,           Y("Edition flag ordered"));
  ADD(KaxChapterAtom,                  Y("Chapter atom"));
  ADD(KaxChapterUID,                   Y("Chapter UID"));
  ADD(KaxChapterStringUID,             Y("Chapter string UID"));
  ADD(KaxChapterTimeStart,             Y("Chapter time start"));
  ADD(KaxChapterTimeEnd,               Y("Chapter time end"));
  ADD(KaxChapterFlagHidden,            Y("Chapter flag hidden"));
  ADD(KaxChapterFlagEnabled,           Y("Chapter flag enabled"));
  ADD(KaxChapterSegmentUID,            Y("Chapter segment UID"));
  ADD(KaxChapterSegmentEditionUID,     Y("Chapter segment edition UID"));
  ADD(KaxChapterPhysicalEquiv,         Y("Chapter physical equivalent"));
  ADD(KaxChapterTrack,                 Y("Chapter track"));
  ADD(KaxChapterTrackNumber,           Y("Chapter track number"));
  ADD(KaxChapterDisplay,               Y("Chapter display"));
  ADD(KaxChapterString,                Y("Chapter string"));
  ADD(KaxChapterLanguage,              Y("Chapter language"));
  ADD(KaxChapLanguageIETF,             Y("Chapter language (IETF BCP 47)"));
  ADD(KaxChapterCountry,               Y("Chapter country"));
  ADD(KaxChapterProcess,               Y("Chapter process"));
  ADD(KaxChapterProcessCodecID,        Y("Chapter process codec ID"));
  ADD(KaxChapterProcessPrivate,        Y("Chapter process private"));
  ADD(KaxChapterProcessCommand,        Y("Chapter process command"));
  ADD(KaxChapterProcessTime,           Y("Chapter process time"));
  ADD(KaxChapterProcessData,           Y("Chapter process data"));

  ADD(KaxTags,                         Y("Tags"));
  ADD(KaxTag,                          Y("Tag"));
  ADD(KaxTagTargets,                   Y("Targets"));
  ADD(KaxTagTrackUID,                  Y("Track UID"));
  ADD(KaxTagEditionUID,                Y("Edition UID"));
  ADD(KaxTagChapterUID,                Y("Chapter UID"));
  ADD(KaxTagAttachmentUID,             Y("Attachment UID"));
  ADD(KaxTagTargetType,                Y("Target type"));
  ADD(KaxTagTargetTypeValue,           Y("Target type value"));
  ADD(KaxTagSimple,                    Y("Simple"));
  ADD(KaxTagName,                      Y("Name"));
  ADD(KaxTagString,                    Y("String"));
  ADD(KaxTagBinary,                    Y("Binary"));
  ADD(KaxTagLangue,                    Y("Tag language"));
  ADD(KaxTagLanguageIETF,              Y("Tag language (IETF BCP 47)"));
  ADD(KaxTagDefault,                   Y("Default language"));
}

}
