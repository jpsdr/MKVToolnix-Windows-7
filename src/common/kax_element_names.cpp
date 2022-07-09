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

  auto add = [](libebml::EbmlCallbacks const &class_infos, char const *description) {
    ms_names.insert({ class_infos.GlobalId.GetValue(), description });
  };

  add(EbmlHead::ClassInfos,                        Y("EBML head"));
  add(EVersion::ClassInfos,                        Y("EBML version"));
  add(EReadVersion::ClassInfos,                    Y("EBML read version"));
  add(EMaxIdLength::ClassInfos,                    Y("Maximum EBML ID length"));
  add(EMaxSizeLength::ClassInfos,                  Y("Maximum EBML size length"));
  add(EDocType::ClassInfos,                        Y("Document type"));
  add(EDocTypeVersion::ClassInfos,                 Y("Document type version"));
  add(EDocTypeReadVersion::ClassInfos,             Y("Document type read version"));
  add(EbmlVoid::ClassInfos,                        Y("EBML void"));
  add(EbmlCrc32::ClassInfos,                       Y("EBML CRC-32"));

  add(KaxSegment::ClassInfos,                      Y("Segment"));
  add(KaxInfo::ClassInfos,                         Y("Segment information"));
  add(KaxTimecodeScale::ClassInfos,                Y("Timestamp scale"));
  add(KaxDuration::ClassInfos,                     Y("Duration"));
  add(KaxMuxingApp::ClassInfos,                    Y("Multiplexing application"));
  add(KaxWritingApp::ClassInfos,                   Y("Writing application"));
  add(KaxDateUTC::ClassInfos,                      Y("Date"));
  add(KaxSegmentUID::ClassInfos,                   Y("Segment UID"));
  add(KaxSegmentFamily::ClassInfos,                Y("Family UID"));
  add(KaxPrevUID::ClassInfos,                      Y("Previous segment UID"));
  add(KaxPrevFilename::ClassInfos,                 Y("Previous filename"));
  add(KaxNextUID::ClassInfos,                      Y("Next segment UID"));
  add(KaxNextFilename::ClassInfos,                 Y("Next filename"));
  add(KaxSegmentFilename::ClassInfos,              Y("Segment filename"));
  add(KaxTitle::ClassInfos,                        Y("Title"));

  add(KaxChapterTranslate::ClassInfos,             Y("Chapter translate"));
  add(KaxChapterTranslateEditionUID::ClassInfos,   Y("Chapter translate edition UID"));
  add(KaxChapterTranslateCodec::ClassInfos,        Y("Chapter translate codec"));
  add(KaxChapterTranslateID::ClassInfos,           Y("Chapter translate ID"));

  add(KaxTrackAudio::ClassInfos,                   Y("Audio track"));
  add(KaxAudioSamplingFreq::ClassInfos,            Y("Sampling frequency"));
  add(KaxAudioOutputSamplingFreq::ClassInfos,      Y("Output sampling frequency"));
  add(KaxAudioChannels::ClassInfos,                Y("Channels"));
  add(KaxAudioPosition::ClassInfos,                Y("Channel positions"));
  add(KaxAudioBitDepth::ClassInfos,                Y("Bit depth"));

  add(KaxVideoColourMasterMeta::ClassInfos,        Y("Video color mastering metadata"));
  add(KaxVideoRChromaX::ClassInfos,                Y("Red color coordinate x"));
  add(KaxVideoRChromaY::ClassInfos,                Y("Red color coordinate y"));
  add(KaxVideoGChromaX::ClassInfos,                Y("Green color coordinate x"));
  add(KaxVideoGChromaY::ClassInfos,                Y("Green color coordinate y"));
  add(KaxVideoBChromaX::ClassInfos,                Y("Blue color coordinate x"));
  add(KaxVideoBChromaY::ClassInfos,                Y("Blue color coordinate y"));
  add(KaxVideoWhitePointChromaX::ClassInfos,       Y("White color coordinate x"));
  add(KaxVideoWhitePointChromaY::ClassInfos,       Y("White color coordinate y"));
  add(KaxVideoLuminanceMax::ClassInfos,            Y("Maximum luminance"));
  add(KaxVideoLuminanceMin::ClassInfos,            Y("Minimum luminance"));

  add(KaxVideoColour::ClassInfos,                  Y("Video color information"));
  add(KaxVideoColourMatrix::ClassInfos,            Y("Color matrix coefficients"));
  add(KaxVideoBitsPerChannel::ClassInfos,          Y("Bits per channel"));
  add(KaxVideoChromaSubsampHorz::ClassInfos,       Y("Horizontal chroma subsample"));
  add(KaxVideoChromaSubsampVert::ClassInfos,       Y("Vertical chroma subsample"));
  add(KaxVideoCbSubsampHorz::ClassInfos,           Y("Horizontal Cb subsample"));
  add(KaxVideoCbSubsampVert::ClassInfos,           Y("Vertical Cb subsample"));
  add(KaxVideoChromaSitHorz::ClassInfos,           Y("Horizontal chroma siting"));
  add(KaxVideoChromaSitVert::ClassInfos,           Y("Vertical chroma siting"));
  add(KaxVideoColourRange::ClassInfos,             Y("Color range"));
  add(KaxVideoColourTransferCharacter::ClassInfos, Y("Color transfer"));
  add(KaxVideoColourPrimaries::ClassInfos,         Y("Color primaries"));
  add(KaxVideoColourMaxCLL::ClassInfos,            Y("Maximum content light"));
  add(KaxVideoColourMaxFALL::ClassInfos,           Y("Maximum frame light"));

  add(KaxVideoProjection::ClassInfos,              Y("Video projection"));
  add(KaxVideoProjectionType::ClassInfos,          Y("Projection type"));
  add(KaxVideoProjectionPrivate::ClassInfos,       Y("Projection's private data"));
  add(KaxVideoProjectionPoseYaw::ClassInfos,       Y("Projection's yaw rotation"));
  add(KaxVideoProjectionPosePitch::ClassInfos,     Y("Projection's pitch rotation"));
  add(KaxVideoProjectionPoseRoll::ClassInfos,      Y("Projection's roll rotation"));

  add(KaxTrackVideo::ClassInfos,                   Y("Video track"));
  add(KaxVideoPixelWidth::ClassInfos,              Y("Pixel width"));
  add(KaxVideoPixelHeight::ClassInfos,             Y("Pixel height"));
  add(KaxVideoDisplayWidth::ClassInfos,            Y("Display width"));
  add(KaxVideoDisplayHeight::ClassInfos,           Y("Display height"));
  add(KaxVideoPixelCropLeft::ClassInfos,           Y("Pixel crop left"));
  add(KaxVideoPixelCropTop::ClassInfos,            Y("Pixel crop top"));
  add(KaxVideoPixelCropRight::ClassInfos,          Y("Pixel crop right"));
  add(KaxVideoPixelCropBottom::ClassInfos,         Y("Pixel crop bottom"));
  add(KaxVideoDisplayUnit::ClassInfos,             Y("Display unit"));
  add(KaxVideoGamma::ClassInfos,                   Y("Gamma"));
  add(KaxVideoFlagInterlaced::ClassInfos,          Y("Interlaced"));
  add(KaxVideoFieldOrder::ClassInfos,              Y("Field order"));
  add(KaxVideoStereoMode::ClassInfos,              Y("Stereo mode"));
  add(KaxVideoAspectRatio::ClassInfos,             Y("Aspect ratio type"));
  add(KaxVideoColourSpace::ClassInfos,             Y("Color space"));
  add(KaxVideoFrameRate::ClassInfos,               Y("Frame rate"));

  add(KaxContentEncodings::ClassInfos,             Y("Content encodings"));
  add(KaxContentEncoding::ClassInfos,              Y("Content encoding"));
  add(KaxContentEncodingOrder::ClassInfos,         Y("Order"));
  add(KaxContentEncodingScope::ClassInfos,         Y("Scope"));
  add(KaxContentEncodingType::ClassInfos,          Y("Type"));
  add(KaxContentCompression::ClassInfos,           Y("Content compression"));
  add(KaxContentCompAlgo::ClassInfos,              Y("Algorithm"));
  add(KaxContentCompSettings::ClassInfos,          Y("Settings"));
  add(KaxContentEncryption::ClassInfos,            Y("Content encryption"));
  add(KaxContentEncAlgo::ClassInfos,               Y("Encryption algorithm"));
  add(KaxContentEncKeyID::ClassInfos,              Y("Encryption key ID"));
  add(KaxContentSigAlgo::ClassInfos,               Y("Signature algorithm"));
  add(KaxContentSigHashAlgo::ClassInfos,           Y("Signature hash algorithm"));
  add(KaxContentSigKeyID::ClassInfos,              Y("Signature key ID"));
  add(KaxContentSignature::ClassInfos,             Y("Signature"));

  add(KaxBlockAdditionMapping::ClassInfos,         Y("Block addition mapping"));
  add(KaxBlockAddIDExtraData::ClassInfos,          Y("Block addition ID extra data"));
  add(KaxBlockAddIDName::ClassInfos,               Y("Block addition ID name"));
  add(KaxBlockAddIDType::ClassInfos,               Y("Block addition ID type"));
  add(KaxBlockAddIDValue::ClassInfos,              Y("Block addition ID value"));

  add(KaxTracks::ClassInfos,                       Y("Tracks"));
  add(KaxTrackEntry::ClassInfos,                   Y("Track"));
  add(KaxTrackNumber::ClassInfos,                  Y("Track number"));
  add(KaxTrackUID::ClassInfos,                     Y("Track UID"));
  add(KaxTrackType::ClassInfos,                    Y("Track type"));
  add(KaxTrackFlagEnabled::ClassInfos,             Y("\"Enabled\" flag"));
  add(KaxTrackName::ClassInfos,                    Y("Name"));
  add(KaxCodecID::ClassInfos,                      Y("Codec ID"));
  add(KaxCodecPrivate::ClassInfos,                 Y("Codec's private data"));
  add(KaxCodecName::ClassInfos,                    Y("Codec name"));
  add(KaxCodecDelay::ClassInfos,                   Y("Codec-inherent delay"));
  add(KaxCodecSettings::ClassInfos,                Y("Codec settings"));
  add(KaxCodecInfoURL::ClassInfos,                 Y("Codec info URL"));
  add(KaxCodecDownloadURL::ClassInfos,             Y("Codec download URL"));
  add(KaxCodecDecodeAll::ClassInfos,               Y("Codec decode all"));
  add(KaxTrackOverlay::ClassInfos,                 Y("Track overlay"));
  add(KaxTrackMinCache::ClassInfos,                Y("Minimum cache"));
  add(KaxTrackMaxCache::ClassInfos,                Y("Maximum cache"));
  add(KaxTrackDefaultDuration::ClassInfos,         Y("Default duration"));
  add(KaxTrackFlagLacing::ClassInfos,              Y("\"Lacing\" flag"));
  add(KaxTrackFlagDefault::ClassInfos,             Y("\"Default track\" flag"));
  add(KaxTrackFlagForced::ClassInfos,              Y("\"Forced display\" flag"));
  add(KaxTrackLanguage::ClassInfos,                Y("Language"));
  add(KaxLanguageIETF::ClassInfos,                 Y("Language (IETF BCP 47)"));
  add(KaxTrackTimecodeScale::ClassInfos,           Y("Timestamp scale"));
  add(KaxMaxBlockAdditionID::ClassInfos,           Y("Maximum block additional ID"));
  add(KaxContentEncodings::ClassInfos,             Y("Content encodings"));
  add(KaxSeekPreRoll::ClassInfos,                  Y("Seek pre-roll"));
  add(KaxFlagHearingImpaired::ClassInfos,          Y("\"Hearing impaired\" flag"));
  add(KaxFlagVisualImpaired::ClassInfos,           Y("\"Visual impaired\" flag"));
  add(KaxFlagTextDescriptions::ClassInfos,         Y("\"Text descriptions\" flag"));
  add(KaxFlagOriginal::ClassInfos,                 Y("\"Original language\" flag"));
  add(KaxFlagCommentary::ClassInfos,               Y("\"Commentary\" flag"));

  add(KaxSeekHead::ClassInfos,                     Y("Seek head"));
  add(KaxSeek::ClassInfos,                         Y("Seek entry"));
  add(KaxSeekID::ClassInfos,                       Y("Seek ID"));
  add(KaxSeekPosition::ClassInfos,                 Y("Seek position"));

  add(KaxCues::ClassInfos,                         Y("Cues"));
  add(KaxCuePoint::ClassInfos,                     Y("Cue point"));
  add(KaxCueTime::ClassInfos,                      Y("Cue time"));
  add(KaxCueTrackPositions::ClassInfos,            Y("Cue track positions"));
  add(KaxCueTrack::ClassInfos,                     Y("Cue track"));
  add(KaxCueClusterPosition::ClassInfos,           Y("Cue cluster position"));
  add(KaxCueRelativePosition::ClassInfos,          Y("Cue relative position"));
  add(KaxCueDuration::ClassInfos,                  Y("Cue duration"));
  add(KaxCueBlockNumber::ClassInfos,               Y("Cue block number"));
  add(KaxCueCodecState::ClassInfos,                Y("Cue codec state"));
  add(KaxCueReference::ClassInfos,                 Y("Cue reference"));
  add(KaxCueRefTime::ClassInfos,                   Y("Cue ref time"));
  add(KaxCueRefCluster::ClassInfos,                Y("Cue ref cluster"));
  add(KaxCueRefNumber::ClassInfos,                 Y("Cue ref number"));
  add(KaxCueRefCodecState::ClassInfos,             Y("Cue ref codec state"));

  add(KaxAttachments::ClassInfos,                  Y("Attachments"));
  add(KaxAttached::ClassInfos,                     Y("Attached"));
  add(KaxFileDescription::ClassInfos,              Y("File description"));
  add(KaxFileName::ClassInfos,                     Y("File name"));
  add(KaxMimeType::ClassInfos,                     Y("MIME type"));
  add(KaxFileData::ClassInfos,                     Y("File data"));
  add(KaxFileUID::ClassInfos,                      Y("File UID"));

  add(KaxClusterSilentTracks::ClassInfos,          Y("Silent tracks"));
  add(KaxClusterSilentTrackNumber::ClassInfos,     Y("Silent track number"));

  add(KaxBlockGroup::ClassInfos,                   Y("Block group"));
  add(KaxBlock::ClassInfos,                        Y("Block"));
  add(KaxBlockDuration::ClassInfos,                Y("Block duration"));
  add(KaxReferenceBlock::ClassInfos,               Y("Reference block"));
  add(KaxReferencePriority::ClassInfos,            Y("Reference priority"));
  add(KaxBlockVirtual::ClassInfos,                 Y("Block virtual"));
  add(KaxReferenceVirtual::ClassInfos,             Y("Reference virtual"));
  add(KaxCodecState::ClassInfos,                   Y("Codec state"));
  add(KaxBlockAddID::ClassInfos,                   Y("Block additional ID"));
  add(KaxBlockAdditional::ClassInfos,              Y("Block additional"));
  add(KaxSliceLaceNumber::ClassInfos,              Y("Lace number"));
  add(KaxSliceFrameNumber::ClassInfos,             Y("Frame number"));
  add(KaxSliceDelay::ClassInfos,                   Y("Delay"));
  add(KaxSliceDuration::ClassInfos,                Y("Duration"));
  add(KaxSliceBlockAddID::ClassInfos,              Y("Block additional ID"));
  add(KaxDiscardPadding::ClassInfos,               Y("Discard padding"));
  add(KaxBlockAdditions::ClassInfos,               Y("Additions"));
  add(KaxBlockMore::ClassInfos,                    Y("More"));
  add(KaxSlices::ClassInfos,                       Y("Slices"));
  add(KaxTimeSlice::ClassInfos,                    Y("Time slice"));
  add(KaxSimpleBlock::ClassInfos,                  Y("Simple block"));

  add(KaxCluster::ClassInfos,                      Y("Cluster"));
  add(KaxClusterTimecode::ClassInfos,              Y("Cluster timestamp"));
  add(KaxClusterPosition::ClassInfos,              Y("Cluster position"));
  add(KaxClusterPrevSize::ClassInfos,              Y("Cluster previous size"));

  add(KaxChapters::ClassInfos,                     Y("Chapters"));
  add(KaxEditionEntry::ClassInfos,                 Y("Edition entry"));
  add(KaxEditionUID::ClassInfos,                   Y("Edition UID"));
  add(KaxEditionFlagHidden::ClassInfos,            Y("Edition flag hidden"));
  add(KaxEditionFlagDefault::ClassInfos,           Y("Edition flag default"));
  add(KaxEditionFlagOrdered::ClassInfos,           Y("Edition flag ordered"));
  add(KaxChapterAtom::ClassInfos,                  Y("Chapter atom"));
  add(KaxChapterUID::ClassInfos,                   Y("Chapter UID"));
  add(KaxChapterStringUID::ClassInfos,             Y("Chapter string UID"));
  add(KaxChapterTimeStart::ClassInfos,             Y("Chapter time start"));
  add(KaxChapterTimeEnd::ClassInfos,               Y("Chapter time end"));
  add(KaxChapterFlagHidden::ClassInfos,            Y("Chapter flag hidden"));
  add(KaxChapterFlagEnabled::ClassInfos,           Y("Chapter flag enabled"));
  add(KaxChapterSegmentUID::ClassInfos,            Y("Chapter segment UID"));
  add(KaxChapterSegmentEditionUID::ClassInfos,     Y("Chapter segment edition UID"));
  add(KaxChapterPhysicalEquiv::ClassInfos,         Y("Chapter physical equivalent"));
  add(KaxChapterTrack::ClassInfos,                 Y("Chapter track"));
  add(KaxChapterTrackNumber::ClassInfos,           Y("Chapter track number"));
  add(KaxChapterDisplay::ClassInfos,               Y("Chapter display"));
  add(KaxChapterString::ClassInfos,                Y("Chapter string"));
  add(KaxChapterLanguage::ClassInfos,              Y("Chapter language"));
  add(KaxChapLanguageIETF::ClassInfos,             Y("Chapter language (IETF BCP 47)"));
  add(KaxChapterCountry::ClassInfos,               Y("Chapter country"));
  add(KaxChapterProcess::ClassInfos,               Y("Chapter process"));
  add(KaxChapterProcessCodecID::ClassInfos,        Y("Chapter process codec ID"));
  add(KaxChapterProcessPrivate::ClassInfos,        Y("Chapter process private"));
  add(KaxChapterProcessCommand::ClassInfos,        Y("Chapter process command"));
  add(KaxChapterProcessTime::ClassInfos,           Y("Chapter process time"));
  add(KaxChapterProcessData::ClassInfos,           Y("Chapter process data"));

  add(KaxTags::ClassInfos,                         Y("Tags"));
  add(KaxTag::ClassInfos,                          Y("Tag"));
  add(KaxTagTargets::ClassInfos,                   Y("Targets"));
  add(KaxTagTrackUID::ClassInfos,                  Y("Track UID"));
  add(KaxTagEditionUID::ClassInfos,                Y("Edition UID"));
  add(KaxTagChapterUID::ClassInfos,                Y("Chapter UID"));
  add(KaxTagAttachmentUID::ClassInfos,             Y("Attachment UID"));
  add(KaxTagTargetType::ClassInfos,                Y("Target type"));
  add(KaxTagTargetTypeValue::ClassInfos,           Y("Target type value"));
  add(KaxTagSimple::ClassInfos,                    Y("Simple"));
  add(KaxTagName::ClassInfos,                      Y("Name"));
  add(KaxTagString::ClassInfos,                    Y("String"));
  add(KaxTagBinary::ClassInfos,                    Y("Binary"));
  add(KaxTagLangue::ClassInfos,                    Y("Tag language"));
  add(KaxTagLanguageIETF::ClassInfos,              Y("Tag language (IETF BCP 47)"));
  add(KaxTagDefault::ClassInfos,                   Y("Default language"));
}

}
