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

void
kax_element_names_c::init() {
  if (!ms_names.empty())
    return;

  auto add = [](libebml::EbmlId const &id, char const *description) {
    ms_names.insert({ id.GetValue(), description });
  };

  add(EBML_ID(EbmlHead),                        Y("EBML head"));
  add(EBML_ID(EVersion),                        Y("EBML version"));
  add(EBML_ID(EReadVersion),                    Y("EBML read version"));
  add(EBML_ID(EMaxIdLength),                    Y("Maximum EBML ID length"));
  add(EBML_ID(EMaxSizeLength),                  Y("Maximum EBML size length"));
  add(EBML_ID(EDocType),                        Y("Document type"));
  add(EBML_ID(EDocTypeVersion),                 Y("Document type version"));
  add(EBML_ID(EDocTypeReadVersion),             Y("Document type read version"));
  add(EBML_ID(EbmlVoid),                        Y("EBML void"));
  add(EBML_ID(EbmlCrc32),                       Y("EBML CRC-32"));

  add(EBML_ID(KaxSegment),                      Y("Segment"));
  add(EBML_ID(KaxInfo),                         Y("Segment information"));
  add(EBML_ID(KaxTimecodeScale),                Y("Timestamp scale"));
  add(EBML_ID(KaxDuration),                     Y("Duration"));
  add(EBML_ID(KaxMuxingApp),                    Y("Multiplexing application"));
  add(EBML_ID(KaxWritingApp),                   Y("Writing application"));
  add(EBML_ID(KaxDateUTC),                      Y("Date"));
  add(EBML_ID(KaxSegmentUID),                   Y("Segment UID"));
  add(EBML_ID(KaxSegmentFamily),                Y("Family UID"));
  add(EBML_ID(KaxPrevUID),                      Y("Previous segment UID"));
  add(EBML_ID(KaxPrevFilename),                 Y("Previous filename"));
  add(EBML_ID(KaxNextUID),                      Y("Next segment UID"));
  add(EBML_ID(KaxNextFilename),                 Y("Next filename"));
  add(EBML_ID(KaxSegmentFilename),              Y("Segment filename"));
  add(EBML_ID(KaxTitle),                        Y("Title"));

  add(EBML_ID(KaxChapterTranslate),             Y("Chapter translate"));
  add(EBML_ID(KaxChapterTranslateEditionUID),   Y("Chapter translate edition UID"));
  add(EBML_ID(KaxChapterTranslateCodec),        Y("Chapter translate codec"));
  add(EBML_ID(KaxChapterTranslateID),           Y("Chapter translate ID"));

  add(EBML_ID(KaxTrackAudio),                   Y("Audio track"));
  add(EBML_ID(KaxAudioSamplingFreq),            Y("Sampling frequency"));
  add(EBML_ID(KaxAudioOutputSamplingFreq),      Y("Output sampling frequency"));
  add(EBML_ID(KaxAudioChannels),                Y("Channels"));
  add(EBML_ID(KaxAudioPosition),                Y("Channel positions"));
  add(EBML_ID(KaxAudioBitDepth),                Y("Bit depth"));
  add(EBML_ID(KaxEmphasis),                     Y("Emphasis"));

  add(EBML_ID(KaxVideoColourMasterMeta),        Y("Video color mastering metadata"));
  add(EBML_ID(KaxVideoRChromaX),                Y("Red color coordinate x"));
  add(EBML_ID(KaxVideoRChromaY),                Y("Red color coordinate y"));
  add(EBML_ID(KaxVideoGChromaX),                Y("Green color coordinate x"));
  add(EBML_ID(KaxVideoGChromaY),                Y("Green color coordinate y"));
  add(EBML_ID(KaxVideoBChromaX),                Y("Blue color coordinate x"));
  add(EBML_ID(KaxVideoBChromaY),                Y("Blue color coordinate y"));
  add(EBML_ID(KaxVideoWhitePointChromaX),       Y("White color coordinate x"));
  add(EBML_ID(KaxVideoWhitePointChromaY),       Y("White color coordinate y"));
  add(EBML_ID(KaxVideoLuminanceMax),            Y("Maximum luminance"));
  add(EBML_ID(KaxVideoLuminanceMin),            Y("Minimum luminance"));

  add(EBML_ID(KaxVideoColour),                  Y("Video color information"));
  add(EBML_ID(KaxVideoColourMatrix),            Y("Color matrix coefficients"));
  add(EBML_ID(KaxVideoBitsPerChannel),          Y("Bits per channel"));
  add(EBML_ID(KaxVideoChromaSubsampHorz),       Y("Horizontal chroma subsample"));
  add(EBML_ID(KaxVideoChromaSubsampVert),       Y("Vertical chroma subsample"));
  add(EBML_ID(KaxVideoCbSubsampHorz),           Y("Horizontal Cb subsample"));
  add(EBML_ID(KaxVideoCbSubsampVert),           Y("Vertical Cb subsample"));
  add(EBML_ID(KaxVideoChromaSitHorz),           Y("Horizontal chroma siting"));
  add(EBML_ID(KaxVideoChromaSitVert),           Y("Vertical chroma siting"));
  add(EBML_ID(KaxVideoColourRange),             Y("Color range"));
  add(EBML_ID(KaxVideoColourTransferCharacter), Y("Color transfer"));
  add(EBML_ID(KaxVideoColourPrimaries),         Y("Color primaries"));
  add(EBML_ID(KaxVideoColourMaxCLL),            Y("Maximum content light"));
  add(EBML_ID(KaxVideoColourMaxFALL),           Y("Maximum frame light"));

  add(EBML_ID(KaxVideoProjection),              Y("Video projection"));
  add(EBML_ID(KaxVideoProjectionType),          Y("Projection type"));
  add(EBML_ID(KaxVideoProjectionPrivate),       Y("Projection's private data"));
  add(EBML_ID(KaxVideoProjectionPoseYaw),       Y("Projection's yaw rotation"));
  add(EBML_ID(KaxVideoProjectionPosePitch),     Y("Projection's pitch rotation"));
  add(EBML_ID(KaxVideoProjectionPoseRoll),      Y("Projection's roll rotation"));

  add(EBML_ID(KaxTrackVideo),                   Y("Video track"));
  add(EBML_ID(KaxVideoPixelWidth),              Y("Pixel width"));
  add(EBML_ID(KaxVideoPixelHeight),             Y("Pixel height"));
  add(EBML_ID(KaxVideoDisplayWidth),            Y("Display width"));
  add(EBML_ID(KaxVideoDisplayHeight),           Y("Display height"));
  add(EBML_ID(KaxVideoPixelCropLeft),           Y("Pixel crop left"));
  add(EBML_ID(KaxVideoPixelCropTop),            Y("Pixel crop top"));
  add(EBML_ID(KaxVideoPixelCropRight),          Y("Pixel crop right"));
  add(EBML_ID(KaxVideoPixelCropBottom),         Y("Pixel crop bottom"));
  add(EBML_ID(KaxVideoDisplayUnit),             Y("Display unit"));
  add(EBML_ID(KaxVideoGamma),                   Y("Gamma"));
  add(EBML_ID(KaxVideoFlagInterlaced),          Y("Interlaced"));
  add(EBML_ID(KaxVideoFieldOrder),              Y("Field order"));
  add(EBML_ID(KaxVideoStereoMode),              Y("Stereo mode"));
  add(EBML_ID(KaxVideoAspectRatio),             Y("Aspect ratio type"));
  add(EBML_ID(KaxVideoColourSpace),             Y("Color space"));
  add(EBML_ID(KaxVideoFrameRate),               Y("Frame rate"));
  add(EBML_ID(KaxOldStereoMode),                Y("Stereo mode (deprecated element)"));

  add(EBML_ID(KaxContentEncodings),             Y("Content encodings"));
  add(EBML_ID(KaxContentEncoding),              Y("Content encoding"));
  add(EBML_ID(KaxContentEncodingOrder),         Y("Order"));
  add(EBML_ID(KaxContentEncodingScope),         Y("Scope"));
  add(EBML_ID(KaxContentEncodingType),          Y("Type"));
  add(EBML_ID(KaxContentCompression),           Y("Content compression"));
  add(EBML_ID(KaxContentCompAlgo),              Y("Algorithm"));
  add(EBML_ID(KaxContentCompSettings),          Y("Settings"));
  add(EBML_ID(KaxContentEncryption),            Y("Content encryption"));
  add(EBML_ID(KaxContentEncAlgo),               Y("Encryption algorithm"));
  add(EBML_ID(KaxContentEncKeyID),              Y("Encryption key ID"));
  add(EBML_ID(KaxContentSigAlgo),               Y("Signature algorithm"));
  add(EBML_ID(KaxContentSigHashAlgo),           Y("Signature hash algorithm"));
  add(EBML_ID(KaxContentSigKeyID),              Y("Signature key ID"));
  add(EBML_ID(KaxContentSignature),             Y("Signature"));

  add(EBML_ID(KaxBlockAdditionMapping),         Y("Block addition mapping"));
  add(EBML_ID(KaxBlockAddIDExtraData),          Y("Block addition ID extra data"));
  add(EBML_ID(KaxBlockAddIDName),               Y("Block addition ID name"));
  add(EBML_ID(KaxBlockAddIDType),               Y("Block addition ID type"));
  add(EBML_ID(KaxBlockAddIDValue),              Y("Block addition ID value"));

  add(EBML_ID(KaxTracks),                       Y("Tracks"));
  add(EBML_ID(KaxTrackEntry),                   Y("Track"));
  add(EBML_ID(KaxTrackNumber),                  Y("Track number"));
  add(EBML_ID(KaxTrackUID),                     Y("Track UID"));
  add(EBML_ID(KaxTrackType),                    Y("Track type"));
  add(EBML_ID(KaxTrackFlagEnabled),             Y("\"Enabled\" flag"));
  add(EBML_ID(KaxTrackName),                    Y("Name"));
  add(EBML_ID(KaxCodecID),                      Y("Codec ID"));
  add(EBML_ID(KaxCodecPrivate),                 Y("Codec's private data"));
  add(EBML_ID(KaxCodecName),                    Y("Codec name"));
  add(EBML_ID(KaxCodecDelay),                   Y("Codec-inherent delay"));
  add(EBML_ID(KaxCodecSettings),                Y("Codec settings"));
  add(EBML_ID(KaxCodecInfoURL),                 Y("Codec info URL"));
  add(EBML_ID(KaxCodecDownloadURL),             Y("Codec download URL"));
  add(EBML_ID(KaxCodecDecodeAll),               Y("Codec decode all"));
  add(EBML_ID(KaxTrackOverlay),                 Y("Track overlay"));
  add(EBML_ID(KaxTrackMinCache),                Y("Minimum cache"));
  add(EBML_ID(KaxTrackMaxCache),                Y("Maximum cache"));
  add(EBML_ID(KaxTrackDefaultDuration),         Y("Default duration"));
  add(EBML_ID(KaxTrackFlagLacing),              Y("\"Lacing\" flag"));
  add(EBML_ID(KaxTrackFlagDefault),             Y("\"Default track\" flag"));
  add(EBML_ID(KaxTrackFlagForced),              Y("\"Forced display\" flag"));
  add(EBML_ID(KaxTrackLanguage),                Y("Language"));
  add(EBML_ID(KaxLanguageIETF),                 Y("Language (IETF BCP 47)"));
  add(EBML_ID(KaxTrackTimecodeScale),           Y("Timestamp scale"));
  add(EBML_ID(KaxMaxBlockAdditionID),           Y("Maximum block additional ID"));
  add(EBML_ID(KaxContentEncodings),             Y("Content encodings"));
  add(EBML_ID(KaxSeekPreRoll),                  Y("Seek pre-roll"));
  add(EBML_ID(KaxFlagHearingImpaired),          Y("\"Hearing impaired\" flag"));
  add(EBML_ID(KaxFlagVisualImpaired),           Y("\"Visual impaired\" flag"));
  add(EBML_ID(KaxFlagTextDescriptions),         Y("\"Text descriptions\" flag"));
  add(EBML_ID(KaxFlagOriginal),                 Y("\"Original language\" flag"));
  add(EBML_ID(KaxFlagCommentary),               Y("\"Commentary\" flag"));

  add(EBML_ID(KaxSeekHead),                     Y("Seek head"));
  add(EBML_ID(KaxSeek),                         Y("Seek entry"));
  add(EBML_ID(KaxSeekID),                       Y("Seek ID"));
  add(EBML_ID(KaxSeekPosition),                 Y("Seek position"));

  add(EBML_ID(KaxCues),                         Y("Cues"));
  add(EBML_ID(KaxCuePoint),                     Y("Cue point"));
  add(EBML_ID(KaxCueTime),                      Y("Cue time"));
  add(EBML_ID(KaxCueTrackPositions),            Y("Cue track positions"));
  add(EBML_ID(KaxCueTrack),                     Y("Cue track"));
  add(EBML_ID(KaxCueClusterPosition),           Y("Cue cluster position"));
  add(EBML_ID(KaxCueRelativePosition),          Y("Cue relative position"));
  add(EBML_ID(KaxCueDuration),                  Y("Cue duration"));
  add(EBML_ID(KaxCueBlockNumber),               Y("Cue block number"));
  add(EBML_ID(KaxCueCodecState),                Y("Cue codec state"));
  add(EBML_ID(KaxCueReference),                 Y("Cue reference"));
  add(EBML_ID(KaxCueRefTime),                   Y("Cue ref time"));
  add(EBML_ID(KaxCueRefCluster),                Y("Cue ref cluster"));
  add(EBML_ID(KaxCueRefNumber),                 Y("Cue ref number"));
  add(EBML_ID(KaxCueRefCodecState),             Y("Cue ref codec state"));

  add(EBML_ID(KaxAttachments),                  Y("Attachments"));
  add(EBML_ID(KaxAttached),                     Y("Attached"));
  add(EBML_ID(KaxFileDescription),              Y("File description"));
  add(EBML_ID(KaxFileName),                     Y("File name"));
  add(EBML_ID(KaxMimeType),                     Y("MIME type"));
  add(EBML_ID(KaxFileData),                     Y("File data"));
  add(EBML_ID(KaxFileUID),                      Y("File UID"));

  add(EBML_ID(KaxClusterSilentTracks),          Y("Silent tracks"));
  add(EBML_ID(KaxClusterSilentTrackNumber),     Y("Silent track number"));

  add(EBML_ID(KaxBlockGroup),                   Y("Block group"));
  add(EBML_ID(KaxBlock),                        Y("Block"));
  add(EBML_ID(KaxBlockDuration),                Y("Block duration"));
  add(EBML_ID(KaxReferenceBlock),               Y("Reference block"));
  add(EBML_ID(KaxReferencePriority),            Y("Reference priority"));
  add(EBML_ID(KaxBlockVirtual),                 Y("Block virtual"));
  add(EBML_ID(KaxReferenceVirtual),             Y("Reference virtual"));
  add(EBML_ID(KaxCodecState),                   Y("Codec state"));
  add(EBML_ID(KaxBlockAddID),                   Y("Block additional ID"));
  add(EBML_ID(KaxBlockAdditional),              Y("Block additional"));
  add(EBML_ID(KaxSliceLaceNumber),              Y("Lace number"));
  add(EBML_ID(KaxSliceFrameNumber),             Y("Frame number"));
  add(EBML_ID(KaxSliceDelay),                   Y("Delay"));
  add(EBML_ID(KaxSliceDuration),                Y("Duration"));
  add(EBML_ID(KaxSliceBlockAddID),              Y("Block additional ID"));
  add(EBML_ID(KaxDiscardPadding),               Y("Discard padding"));
  add(EBML_ID(KaxBlockAdditions),               Y("Additions"));
  add(EBML_ID(KaxBlockMore),                    Y("More"));
  add(EBML_ID(KaxSlices),                       Y("Slices"));
  add(EBML_ID(KaxTimeSlice),                    Y("Time slice"));
  add(EBML_ID(KaxSimpleBlock),                  Y("Simple block"));

  add(EBML_ID(KaxCluster),                      Y("Cluster"));
  add(EBML_ID(KaxClusterTimecode),              Y("Cluster timestamp"));
  add(EBML_ID(KaxClusterPosition),              Y("Cluster position"));
  add(EBML_ID(KaxClusterPrevSize),              Y("Cluster previous size"));

  add(EBML_ID(KaxChapters),                     Y("Chapters"));
  add(EBML_ID(KaxEditionEntry),                 Y("Edition entry"));
  add(EBML_ID(KaxEditionUID),                   Y("Edition UID"));
  add(EBML_ID(KaxEditionFlagHidden),            Y("Edition flag hidden"));
  add(EBML_ID(KaxEditionFlagDefault),           Y("Edition flag default"));
  add(EBML_ID(KaxEditionFlagOrdered),           Y("Edition flag ordered"));
  add(EBML_ID(KaxEditionDisplay),               Y("Edition display"));
  add(EBML_ID(KaxEditionString),                Y("Edition string"));
  add(EBML_ID(KaxEditionLanguageIETF),          Y("Edition language (IETF BCP 47)"));
  add(EBML_ID(KaxChapterAtom),                  Y("Chapter atom"));
  add(EBML_ID(KaxChapterUID),                   Y("Chapter UID"));
  add(EBML_ID(KaxChapterStringUID),             Y("Chapter string UID"));
  add(EBML_ID(KaxChapterTimeStart),             Y("Chapter time start"));
  add(EBML_ID(KaxChapterTimeEnd),               Y("Chapter time end"));
  add(EBML_ID(KaxChapterFlagHidden),            Y("Chapter flag hidden"));
  add(EBML_ID(KaxChapterFlagEnabled),           Y("Chapter flag enabled"));
  add(EBML_ID(KaxChapterSegmentUID),            Y("Chapter segment UID"));
  add(EBML_ID(KaxChapterSegmentEditionUID),     Y("Chapter segment edition UID"));
  add(EBML_ID(KaxChapterPhysicalEquiv),         Y("Chapter physical equivalent"));
  add(EBML_ID(KaxChapterTrack),                 Y("Chapter track"));
  add(EBML_ID(KaxChapterTrackNumber),           Y("Chapter track number"));
  add(EBML_ID(KaxChapterDisplay),               Y("Chapter display"));
  add(EBML_ID(KaxChapterString),                Y("Chapter string"));
  add(EBML_ID(KaxChapterLanguage),              Y("Chapter language"));
  add(EBML_ID(KaxChapLanguageIETF),             Y("Chapter language (IETF BCP 47)"));
  add(EBML_ID(KaxChapterCountry),               Y("Chapter country"));
  add(EBML_ID(KaxChapterProcess),               Y("Chapter process"));
  add(EBML_ID(KaxChapterProcessCodecID),        Y("Chapter process codec ID"));
  add(EBML_ID(KaxChapterProcessPrivate),        Y("Chapter process private"));
  add(EBML_ID(KaxChapterProcessCommand),        Y("Chapter process command"));
  add(EBML_ID(KaxChapterProcessTime),           Y("Chapter process time"));
  add(EBML_ID(KaxChapterProcessData),           Y("Chapter process data"));
  add(EBML_ID(KaxChapterSkipType),              Y("Chapter skip type"));

  add(EBML_ID(KaxTags),                         Y("Tags"));
  add(EBML_ID(KaxTag),                          Y("Tag"));
  add(EBML_ID(KaxTagTargets),                   Y("Targets"));
  add(EBML_ID(KaxTagTrackUID),                  Y("Track UID"));
  add(EBML_ID(KaxTagEditionUID),                Y("Edition UID"));
  add(EBML_ID(KaxTagChapterUID),                Y("Chapter UID"));
  add(EBML_ID(KaxTagAttachmentUID),             Y("Attachment UID"));
  add(EBML_ID(KaxTagTargetType),                Y("Target type"));
  add(EBML_ID(KaxTagTargetTypeValue),           Y("Target type value"));
  add(EBML_ID(KaxTagSimple),                    Y("Simple"));
  add(EBML_ID(KaxTagName),                      Y("Name"));
  add(EBML_ID(KaxTagString),                    Y("String"));
  add(EBML_ID(KaxTagBinary),                    Y("Binary"));
  add(EBML_ID(KaxTagLangue),                    Y("Tag language"));
  add(EBML_ID(KaxTagLanguageIETF),              Y("Tag language (IETF BCP 47)"));
  add(EBML_ID(KaxTagDefault),                   Y("Default language"));
  add(EBML_ID(KaxTagDefaultBogus),              Y("Default language (bogus element with invalid ID)"));
}

}
