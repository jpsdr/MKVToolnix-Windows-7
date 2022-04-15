#include "element_info.h"

std::map<uint32_t, std::string> g_element_names;

void
init_element_names() {
  g_element_names[0xbf]       = "EBML::Crc32";
  g_element_names[0x4282]     = "EBML::DocType";
  g_element_names[0x4285]     = "EBML::DocTypeReadVersion";
  g_element_names[0x4287]     = "EBML::DocTypeVersion";
  g_element_names[0x1a45dfa3] = "EBML::Head";
  g_element_names[0x42f2]     = "EBML::MaxIdLength";
  g_element_names[0x42f3]     = "EBML::MaxSizeLength";
  g_element_names[0x42f7]     = "EBML::ReadVersion";
  g_element_names[0x4286]     = "EBML::Version";
  g_element_names[0xec]       = "EBML::Void";

  g_element_names[0x61a7]     = "Attached";
  g_element_names[0x1941a469] = "Attachments";
  g_element_names[0x6264]     = "AudioBitDepth";
  g_element_names[0x9f]       = "AudioChannels";
  g_element_names[0x78b5]     = "AudioOutputSamplingFreq";
  g_element_names[0x7d7b]     = "AudioPosition";
  g_element_names[0xb5]       = "AudioSamplingFreq";
  g_element_names[0xa1]       = "Block";
  g_element_names[0xee]       = "BlockAddID";
  g_element_names[0xa5]       = "BlockAdditional";
  g_element_names[0x75a1]     = "BlockAdditions";
  g_element_names[0x9b]       = "BlockDuration";
  g_element_names[0xa0]       = "BlockGroup";
  g_element_names[0xa6]       = "BlockMore";
  g_element_names[0xa2]       = "BlockVirtual";
  g_element_names[0xb6]       = "ChapterAtom";
  g_element_names[0x437e]     = "ChapterCountry";
  g_element_names[0x80]       = "ChapterDisplay";
  g_element_names[0x4598]     = "ChapterFlagEnabled";
  g_element_names[0x98]       = "ChapterFlagHidden";
  g_element_names[0x437c]     = "ChapterLanguage";
  g_element_names[0x63c3]     = "ChapterPhysicalEquiv";
  g_element_names[0x6944]     = "ChapterProcess";
  g_element_names[0x6955]     = "ChapterProcessCodecID";
  g_element_names[0x6911]     = "ChapterProcessCommand";
  g_element_names[0x6933]     = "ChapterProcessData";
  g_element_names[0x450d]     = "ChapterProcessPrivate";
  g_element_names[0x6922]     = "ChapterProcessTime";
  g_element_names[0x1043a770] = "Chapters";
  g_element_names[0x6ebc]     = "ChapterSegmentEditionUID";
  g_element_names[0x6e67]     = "ChapterSegmentUID";
  g_element_names[0x85]       = "ChapterString";
  g_element_names[0x92]       = "ChapterTimeEnd";
  g_element_names[0x91]       = "ChapterTimeStart";
  g_element_names[0x8f]       = "ChapterTrack";
  g_element_names[0x89]       = "ChapterTrackNumber";
  g_element_names[0x6924]     = "ChapterTranslate";
  g_element_names[0x69bf]     = "ChapterTranslateCodec";
  g_element_names[0x69fc]     = "ChapterTranslateEditionUID";
  g_element_names[0x69a5]     = "ChapterTranslateID";
  g_element_names[0x73c4]     = "ChapterUID";
  g_element_names[0x1f43b675] = "Cluster";
  g_element_names[0xa7]       = "ClusterPosition";
  g_element_names[0xab]       = "ClusterPrevSize";
  g_element_names[0x58d7]     = "ClusterSilentTrackNumber";
  g_element_names[0x5854]     = "ClusterSilentTracks";
  g_element_names[0xe7]       = "ClusterTimecode";
  g_element_names[0xaa]       = "CodecDecodeAll";
  g_element_names[0x26b240]   = "CodecDownloadURL";
  g_element_names[0x86]       = "CodecID";
  g_element_names[0x3b4040]   = "CodecInfoURL";
  g_element_names[0x258688]   = "CodecName";
  g_element_names[0x63a2]     = "CodecPrivate";
  g_element_names[0x3a9697]   = "CodecSettings";
  g_element_names[0xa4]       = "CodecState";
  g_element_names[0x4254]     = "ContentCompAlgo";
  g_element_names[0x5034]     = "ContentCompression";
  g_element_names[0x4255]     = "ContentCompSettings";
  g_element_names[0x47e1]     = "ContentEncAlgo";
  g_element_names[0x47e2]     = "ContentEncKeyID";
  g_element_names[0x6240]     = "ContentEncoding";
  g_element_names[0x5031]     = "ContentEncodingOrder";
  g_element_names[0x6d80]     = "ContentEncodings";
  g_element_names[0x5032]     = "ContentEncodingScope";
  g_element_names[0x5033]     = "ContentEncodingType";
  g_element_names[0x5035]     = "ContentEncryption";
  g_element_names[0x47e5]     = "ContentSigAlgo";
  g_element_names[0x47e6]     = "ContentSigHashAlgo";
  g_element_names[0x47e4]     = "ContentSigKeyID";
  g_element_names[0x47e3]     = "ContentSignature";
  g_element_names[0x5378]     = "CueBlockNumber";
  g_element_names[0xf1]       = "CueClusterPosition";
  g_element_names[0xea]       = "CueCodecState";
  g_element_names[0xbb]       = "CuePoint";
  g_element_names[0x97]       = "CueRefCluster";
  g_element_names[0xeb]       = "CueRefCodecState";
  g_element_names[0xdb]       = "CueReference";
  g_element_names[0x535f]     = "CueRefNumber";
  g_element_names[0x96]       = "CueRefTime";
  g_element_names[0x1c53bb6b] = "Cues";
  g_element_names[0xb3]       = "CueTime";
  g_element_names[0xf7]       = "CueTrack";
  g_element_names[0xb7]       = "CueTrackPositions";
  g_element_names[0x4461]     = "DateUTC";
  g_element_names[0x4489]     = "Duration";
  g_element_names[0x45b9]     = "EditionEntry";
  g_element_names[0x45db]     = "EditionFlagDefault";
  g_element_names[0x45bd]     = "EditionFlagHidden";
  g_element_names[0x45dd]     = "EditionFlagOrdered";
  g_element_names[0x45bc]     = "EditionUID";
  g_element_names[0x465c]     = "FileData";
  g_element_names[0x467e]     = "FileDescription";
  g_element_names[0x466e]     = "FileName";
  g_element_names[0x4675]     = "FileReferral";
  g_element_names[0x46ae]     = "FileUID";
  g_element_names[0x1549a966] = "Info";
  g_element_names[0x55ee]     = "MaxBlockAdditionID";
  g_element_names[0x4660]     = "MimeType";
  g_element_names[0x4d80]     = "MuxingApp";
  g_element_names[0x3e83bb]   = "NextFilename";
  g_element_names[0x3eb923]   = "NextUID";
  g_element_names[0x3c83ab]   = "PrevFilename";
  g_element_names[0x3cb923]   = "PrevUID";
  g_element_names[0xfb]       = "ReferenceBlock";
  g_element_names[0xfa]       = "ReferencePriority";
  g_element_names[0xfd]       = "ReferenceVirtual";
  g_element_names[0x4dbb]     = "Seek";
  g_element_names[0x114d9b74] = "SeekHead";
  g_element_names[0x53ab]     = "SeekID";
  g_element_names[0x53ac]     = "SeekPosition";
  g_element_names[0x18538067] = "Segment";
  g_element_names[0x4444]     = "SegmentFamily";
  g_element_names[0x7384]     = "SegmentFilename";
  g_element_names[0x73a4]     = "SegmentUID";
  g_element_names[0xa3]       = "SimpleBlock";
  g_element_names[0xcb]       = "SliceBlockAddID";
  g_element_names[0xce]       = "SliceDelay";
  g_element_names[0xcf]       = "SliceDuration";
  g_element_names[0xcd]       = "SliceFrameNumber";
  g_element_names[0xcc]       = "SliceLaceNumber";
  g_element_names[0x8e]       = "Slices";
  g_element_names[0x7373]     = "Tag";
  g_element_names[0x45a4]     = "TagArchivalLocation";
  g_element_names[0x4ec3]     = "TagAttachment";
  g_element_names[0x5ba0]     = "TagAttachmentID";
  g_element_names[0x63c6]     = "TagAttachmentUID";
  g_element_names[0x41b4]     = "TagAudioEncryption";
  g_element_names[0x4199]     = "TagAudioGain";
  g_element_names[0x65c2]     = "TagAudioGenre";
  g_element_names[0x4189]     = "TagAudioPeak";
  g_element_names[0x41c5]     = "TagAudioSpecific";
  g_element_names[0x4488]     = "TagBibliography";
  g_element_names[0x4485]     = "TagBinary";
  g_element_names[0x41a1]     = "TagBPM";
  g_element_names[0x49c7]     = "TagCaptureDPI";
  g_element_names[0x49e1]     = "TagCaptureLightness";
  g_element_names[0x4934]     = "TagCapturePaletteSetting";
  g_element_names[0x4922]     = "TagCaptureSharpness";
  g_element_names[0x63c4]     = "TagChapterUID";
  g_element_names[0x4ec7]     = "TagCommercial";
  g_element_names[0x4987]     = "TagCropped";
  g_element_names[0x4ec8]     = "TagDate";
  g_element_names[0x4484]     = "TagDefault";
  g_element_names[0x41b6]     = "TagDiscTrack";
  g_element_names[0x63c9]     = "TagEditionUID";
  g_element_names[0x4431]     = "TagEncoder";
  g_element_names[0x6526]     = "TagEncodeSettings";
  g_element_names[0x4ec9]     = "TagEntity";
  g_element_names[0x41b1]     = "TagEqualisation";
  g_element_names[0x454e]     = "TagFile";
  g_element_names[0x67c9]     = "TagGeneral";
  g_element_names[0x6583]     = "TagGenres";
  g_element_names[0x4ec6]     = "TagIdentifier";
  g_element_names[0x4990]     = "TagImageSpecific";
  g_element_names[0x413a]     = "TagInitialKey";
  g_element_names[0x458c]     = "TagKeywords";
  g_element_names[0x22b59f]   = "TagLanguage";
  g_element_names[0x447a]     = "TagLangue";
  g_element_names[0x4ec5]     = "TagLegal";
  g_element_names[0x5243]     = "TagLength";
  g_element_names[0x45ae]     = "TagMood";
  g_element_names[0x4dc3]     = "TagMultiAttachment";
  g_element_names[0x5b7b]     = "TagMultiComment";
  g_element_names[0x5f7c]     = "TagMultiCommentComments";
  g_element_names[0x22b59d]   = "TagMultiCommentLanguage";
  g_element_names[0x5f7d]     = "TagMultiCommentName";
  g_element_names[0x4dc7]     = "TagMultiCommercial";
  g_element_names[0x5bbb]     = "TagMultiCommercialAddress";
  g_element_names[0x5bc0]     = "TagMultiCommercialEmail";
  g_element_names[0x5bd7]     = "TagMultiCommercialType";
  g_element_names[0x5bda]     = "TagMultiCommercialURL";
  g_element_names[0x4dc8]     = "TagMultiDate";
  g_element_names[0x4460]     = "TagMultiDateDateBegin";
  g_element_names[0x4462]     = "TagMultiDateDateEnd";
  g_element_names[0x5bd8]     = "TagMultiDateType";
  g_element_names[0x4dc9]     = "TagMultiEntity";
  g_element_names[0x5bdc]     = "TagMultiEntityAddress";
  g_element_names[0x5bc1]     = "TagMultiEntityEmail";
  g_element_names[0x5bed]     = "TagMultiEntityName";
  g_element_names[0x5bd9]     = "TagMultiEntityType";
  g_element_names[0x5bdb]     = "TagMultiEntityURL";
  g_element_names[0x4dc6]     = "TagMultiIdentifier";
  g_element_names[0x6b67]     = "TagMultiIdentifierBinary";
  g_element_names[0x6b68]     = "TagMultiIdentifierString";
  g_element_names[0x5bad]     = "TagMultiIdentifierType";
  g_element_names[0x4dc5]     = "TagMultiLegal";
  g_element_names[0x5b9b]     = "TagMultiLegalAddress";
  g_element_names[0x5bb2]     = "TagMultiLegalContent";
  g_element_names[0x5bbd]     = "TagMultiLegalType";
  g_element_names[0x5b34]     = "TagMultiLegalURL";
  g_element_names[0x5bc3]     = "TagMultiPrice";
  g_element_names[0x5b6e]     = "TagMultiPriceAmount";
  g_element_names[0x5b6c]     = "TagMultiPriceCurrency";
  g_element_names[0x5b6f]     = "TagMultiPricePriceDate";
  g_element_names[0x4dc4]     = "TagMultiTitle";
  g_element_names[0x5b33]     = "TagMultiTitleAddress";
  g_element_names[0x5bae]     = "TagMultiTitleEdition";
  g_element_names[0x5bc9]     = "TagMultiTitleEmail";
  g_element_names[0x22b59e]   = "TagMultiTitleLanguage";
  g_element_names[0x5bb9]     = "TagMultiTitleName";
  g_element_names[0x5b5b]     = "TagMultiTitleSubTitle";
  g_element_names[0x5b7d]     = "TagMultiTitleType";
  g_element_names[0x5ba9]     = "TagMultiTitleURL";
  g_element_names[0x45a3]     = "TagName";
  g_element_names[0x4133]     = "TagOfficialAudioFileURL";
  g_element_names[0x413e]     = "TagOfficialAudioSourceURL";
  g_element_names[0x4933]     = "TagOriginalDimensions";
  g_element_names[0x45a7]     = "TagOriginalMediaType";
  g_element_names[0x4566]     = "TagPlayCounter";
  g_element_names[0x72cc]     = "TagPlaylistDelay";
  g_element_names[0x4532]     = "TagPopularimeter";
  g_element_names[0x45e3]     = "TagProduct";
  g_element_names[0x52bc]     = "TagRating";
  g_element_names[0x457e]     = "TagRecordLocation";
  g_element_names[0x1254c367] = "Tags";
  g_element_names[0x416e]     = "TagSetPart";
  g_element_names[0x67c8]     = "TagSimple";
  g_element_names[0x458a]     = "TagSource";
  g_element_names[0x45b5]     = "TagSourceForm";
  g_element_names[0x4487]     = "TagString";
  g_element_names[0x65ac]     = "TagSubGenre";
  g_element_names[0x49c1]     = "TagSubject";
  g_element_names[0x63c0]     = "TagTargets";
  g_element_names[0x63ca]     = "TagTargetType";
  g_element_names[0x68ca]     = "TagTargetTypeValue";
  g_element_names[0x4ec4]     = "TagTitle";
  g_element_names[0x63c5]     = "TagTrackUID";
  g_element_names[0x874b]     = "TagUnsynchronisedText";
  g_element_names[0x434a]     = "TagUserDefinedURL";
  g_element_names[0x65a1]     = "TagVideoGenre";
  g_element_names[0x2ad7b1]   = "TimecodeScale";
  g_element_names[0xe8]       = "TimeSlice";
  g_element_names[0x7ba9]     = "Title";
  g_element_names[0x7446]     = "TrackAttachmentLink";
  g_element_names[0xe1]       = "TrackAudio";
  g_element_names[0x23e383]   = "TrackDefaultDuration";
  g_element_names[0xae]       = "TrackEntry";
  g_element_names[0x88]       = "TrackFlagDefault";
  g_element_names[0xb9]       = "TrackFlagEnabled";
  g_element_names[0x55aa]     = "TrackFlagForced";
  g_element_names[0x9c]       = "TrackFlagLacing";
  g_element_names[0x22b59c]   = "TrackLanguage";
  g_element_names[0x6df8]     = "TrackMaxCache";
  g_element_names[0x6de7]     = "TrackMinCache";
  g_element_names[0x536e]     = "TrackName";
  g_element_names[0xd7]       = "TrackNumber";
  g_element_names[0x6fab]     = "TrackOverlay";
  g_element_names[0x1654ae6b] = "Tracks";
  g_element_names[0x23314f]   = "TrackTimecodeScale";
  g_element_names[0x6624]     = "TrackTranslate";
  g_element_names[0x66bf]     = "TrackTranslateCodec";
  g_element_names[0x66fc]     = "TrackTranslateEditionUID";
  g_element_names[0x66a5]     = "TrackTranslateTrackID";
  g_element_names[0x83]       = "TrackType";
  g_element_names[0x73c5]     = "TrackUID";
  g_element_names[0xe0]       = "TrackVideo";
  g_element_names[0x54b3]     = "VideoAspectRatio";
  g_element_names[0x2eb524]   = "VideoColourSpace";
  g_element_names[0x54ba]     = "VideoDisplayHeight";
  g_element_names[0x54b2]     = "VideoDisplayUnit";
  g_element_names[0x54b0]     = "VideoDisplayWidth";
  g_element_names[0x9a]       = "VideoFlagInterlaced";
  g_element_names[0x2383e3]   = "VideoFrameRate";
  g_element_names[0x2fb523]   = "VideoGamma";
  g_element_names[0x54aa]     = "VideoPixelCropBottom";
  g_element_names[0x54cc]     = "VideoPixelCropLeft";
  g_element_names[0x54dd]     = "VideoPixelCropRight";
  g_element_names[0x54bb]     = "VideoPixelCropTop";
  g_element_names[0xba]       = "VideoPixelHeight";
  g_element_names[0xb0]       = "VideoPixelWidth";
  g_element_names[0x53b8]     = "VideoStereoMode";
  g_element_names[0x5741]     = "WritingApp";
}

uint32_t
element_name_to_id(const std::string &name) {
  std::map<uint32_t, std::string>::const_iterator i;
  for (i = g_element_names.begin(); g_element_names.end() != i; ++i)
    if (i->second == name)
      return i->first;

  return 0;
}

std::map<uint32_t, bool> g_master_information;

void
init_master_information() {
  const char *s_master_names[] = {
    "Attached",
    "Attachments",
    "TimeSlice",
    "Slices",
    "BlockGroup",
    "BlockAdditions",
    "BlockMore",
    "Chapters",
    "EditionEntry",
    "ChapterAtom",
    "ChapterTrack",
    "ChapterDisplay",
    "ChapterProcess",
    "ChapterProcessCommand",
    "ClusterSilentTracks",
    "Cluster",
    "ContentEncodings",
    "ContentEncoding",
    "ContentCompression",
    "ContentEncryption",
    "CuePoint",
    "CueTrackPositions",
    "CueReference",
    "Cues",
    "ChapterTranslate",
    "Info",
    "SeekHead",
    "Seek",
    "Segment",
    "Tag",
    "TagTargets",
    "TagGeneral",
    "TagGenres",
    "TagAudioSpecific",
    "TagImageSpecific",
    "TagSimple",
    "TagMultiComment",
    "TagMultiCommercial",
    "TagCommercial",
    "TagMultiPrice",
    "TagMultiDate",
    "TagDate",
    "TagMultiEntity",
    "TagEntity",
    "TagMultiIdentifier",
    "TagIdentifier",
    "TagMultiLegal",
    "TagLegal",
    "TagMultiTitle",
    "TagTitle",
    "TagMultiAttachment",
    "TagAttachment",
    "Tags",
    "TrackAudio",
    "TrackTranslate",
    "Tracks",
    "TrackEntry",
    "TrackVideo",
    nullptr
  };

  int i;
  for (i = 0; s_master_names[i]; ++i) {
    uint32_t id = element_name_to_id(s_master_names[i]);
    if (0 != id)
      g_master_information[id] = true;
  }
}
