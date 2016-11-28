#include "common/common_pch.h"

#include "common/codec.h"
#include "common/fourcc.h"
#include "common/mp4.h"

#include "gtest/gtest.h"

namespace {

TEST(Codec, LookUpStringAudio) {
  EXPECT_TRUE(codec_c::look_up(MKV_A_AAC_2MAIN).is(codec_c::type_e::A_AAC));
  EXPECT_TRUE(codec_c::look_up(MKV_A_AAC_2LC).is(codec_c::type_e::A_AAC));
  EXPECT_TRUE(codec_c::look_up(MKV_A_AAC_2SSR).is(codec_c::type_e::A_AAC));
  EXPECT_TRUE(codec_c::look_up(MKV_A_AAC_2SBR).is(codec_c::type_e::A_AAC));
  EXPECT_TRUE(codec_c::look_up(MKV_A_AAC_4MAIN).is(codec_c::type_e::A_AAC));
  EXPECT_TRUE(codec_c::look_up(MKV_A_AAC_4LC).is(codec_c::type_e::A_AAC));
  EXPECT_TRUE(codec_c::look_up(MKV_A_AAC_4LC).is(codec_c::type_e::A_AAC));
  EXPECT_TRUE(codec_c::look_up(MKV_A_AAC_4SSR).is(codec_c::type_e::A_AAC));
  EXPECT_TRUE(codec_c::look_up(MKV_A_AAC_4LTP).is(codec_c::type_e::A_AAC));
  EXPECT_TRUE(codec_c::look_up(MKV_A_AAC_4SBR).is(codec_c::type_e::A_AAC));
  EXPECT_TRUE(codec_c::look_up(MKV_A_AAC).is(codec_c::type_e::A_AAC));
  EXPECT_TRUE(codec_c::look_up("mp4a").is(codec_c::type_e::A_AAC));
  EXPECT_TRUE(codec_c::look_up("aac ").is(codec_c::type_e::A_AAC));
  EXPECT_TRUE(codec_c::look_up("aacl").is(codec_c::type_e::A_AAC));
  EXPECT_TRUE(codec_c::look_up("aach").is(codec_c::type_e::A_AAC));
  EXPECT_TRUE(codec_c::look_up("raac").is(codec_c::type_e::A_AAC));
  EXPECT_TRUE(codec_c::look_up("racp").is(codec_c::type_e::A_AAC));

  EXPECT_TRUE(codec_c::look_up(MKV_A_AC3).is(codec_c::type_e::A_AC3));
  EXPECT_TRUE(codec_c::look_up(MKV_A_EAC3).is(codec_c::type_e::A_AC3));
  EXPECT_TRUE(codec_c::look_up("a52 ").is(codec_c::type_e::A_AC3));
  EXPECT_TRUE(codec_c::look_up("a52b").is(codec_c::type_e::A_AC3));
  EXPECT_TRUE(codec_c::look_up("ac-3").is(codec_c::type_e::A_AC3));
  EXPECT_TRUE(codec_c::look_up("sac3").is(codec_c::type_e::A_AC3));
  EXPECT_TRUE(codec_c::look_up("dnet").is(codec_c::type_e::A_AC3));

  EXPECT_TRUE(codec_c::look_up(MKV_A_ALAC).is(codec_c::type_e::A_ALAC));
  EXPECT_TRUE(codec_c::look_up("ALAC").is(codec_c::type_e::A_ALAC));

  EXPECT_TRUE(codec_c::look_up(MKV_A_DTS).is(codec_c::type_e::A_DTS));
  EXPECT_TRUE(codec_c::look_up("dts ").is(codec_c::type_e::A_DTS));
  EXPECT_TRUE(codec_c::look_up("dtsb").is(codec_c::type_e::A_DTS));
  EXPECT_TRUE(codec_c::look_up("dtsc").is(codec_c::type_e::A_DTS));
  EXPECT_TRUE(codec_c::look_up("dtse").is(codec_c::type_e::A_DTS));
  EXPECT_TRUE(codec_c::look_up("dtsh").is(codec_c::type_e::A_DTS));
  EXPECT_TRUE(codec_c::look_up("dtsl").is(codec_c::type_e::A_DTS));

  EXPECT_TRUE(codec_c::look_up(MKV_A_MP2).is(codec_c::type_e::A_MP2));
  EXPECT_TRUE(codec_c::look_up("mp2a").is(codec_c::type_e::A_MP2));
  EXPECT_TRUE(codec_c::look_up(".mp1").is(codec_c::type_e::A_MP2));
  EXPECT_TRUE(codec_c::look_up(".mp2").is(codec_c::type_e::A_MP2));

  EXPECT_TRUE(codec_c::look_up(MKV_A_MP3).is(codec_c::type_e::A_MP3));
  EXPECT_TRUE(codec_c::look_up("mp3a").is(codec_c::type_e::A_MP3));
  EXPECT_TRUE(codec_c::look_up(".mp3").is(codec_c::type_e::A_MP3));

  EXPECT_TRUE(codec_c::look_up(MKV_A_PCM).is(codec_c::type_e::A_PCM));
  EXPECT_TRUE(codec_c::look_up(MKV_A_PCM_BE).is(codec_c::type_e::A_PCM));
  EXPECT_TRUE(codec_c::look_up(MKV_A_PCM_FLOAT).is(codec_c::type_e::A_PCM));
  EXPECT_TRUE(codec_c::look_up("twos").is(codec_c::type_e::A_PCM));
  EXPECT_TRUE(codec_c::look_up("sowt").is(codec_c::type_e::A_PCM));

  EXPECT_TRUE(codec_c::look_up(MKV_A_VORBIS).is(codec_c::type_e::A_VORBIS));
  EXPECT_TRUE(codec_c::look_up("vorb").is(codec_c::type_e::A_VORBIS));
  EXPECT_TRUE(codec_c::look_up("vor1").is(codec_c::type_e::A_VORBIS));

  EXPECT_TRUE(codec_c::look_up(MKV_A_OPUS).is(codec_c::type_e::A_OPUS));
  EXPECT_TRUE(codec_c::look_up(std::string{MKV_A_OPUS} + "/EXPERIMENTAL").is(codec_c::type_e::A_OPUS));
  EXPECT_TRUE(codec_c::look_up("opus").is(codec_c::type_e::A_OPUS));

  EXPECT_TRUE(codec_c::look_up(MKV_A_QDMC).is(codec_c::type_e::A_QDMC));
  EXPECT_TRUE(codec_c::look_up(MKV_A_QDMC2).is(codec_c::type_e::A_QDMC));
  EXPECT_TRUE(codec_c::look_up("qdm2").is(codec_c::type_e::A_QDMC));

  EXPECT_TRUE(codec_c::look_up(MKV_A_FLAC).is(codec_c::type_e::A_FLAC));
  EXPECT_TRUE(codec_c::look_up("flac").is(codec_c::type_e::A_FLAC));

  EXPECT_TRUE(codec_c::look_up(MKV_A_MLP).is(codec_c::type_e::A_MLP));
  EXPECT_TRUE(codec_c::look_up("mlp ").is(codec_c::type_e::A_MLP));

  EXPECT_TRUE(codec_c::look_up(MKV_A_TRUEHD).is(codec_c::type_e::A_TRUEHD));
  EXPECT_TRUE(codec_c::look_up("trhd").is(codec_c::type_e::A_TRUEHD));

  EXPECT_TRUE(codec_c::look_up(MKV_A_TTA).is(codec_c::type_e::A_TTA));
  EXPECT_TRUE(codec_c::look_up("tta1").is(codec_c::type_e::A_TTA));

  EXPECT_TRUE(codec_c::look_up(MKV_A_WAVPACK4).is(codec_c::type_e::A_WAVPACK4));
  EXPECT_TRUE(codec_c::look_up("wvpk").is(codec_c::type_e::A_WAVPACK4));

  EXPECT_TRUE(codec_c::look_up("A_REAL/COOK").is(codec_c::type_e::A_COOK));
  EXPECT_TRUE(codec_c::look_up("cook").is(codec_c::type_e::A_COOK));

  EXPECT_TRUE(codec_c::look_up("A_REAL/SIPR").is(codec_c::type_e::A_ACELP_NET));
  EXPECT_TRUE(codec_c::look_up("sipr").is(codec_c::type_e::A_ACELP_NET));

  EXPECT_TRUE(codec_c::look_up("A_REAL/ATRC").is(codec_c::type_e::A_ATRAC3));
  EXPECT_TRUE(codec_c::look_up("atrc").is(codec_c::type_e::A_ATRAC3));

  EXPECT_TRUE(codec_c::look_up("A_REAL/RALF").is(codec_c::type_e::A_RALF));
  EXPECT_TRUE(codec_c::look_up("ralf").is(codec_c::type_e::A_RALF));

  EXPECT_TRUE(codec_c::look_up("A_REAL/14_4").is(codec_c::type_e::A_VSELP));
  EXPECT_TRUE(codec_c::look_up("A_REAL/LPCJ").is(codec_c::type_e::A_VSELP));
  EXPECT_TRUE(codec_c::look_up("14_4").is(codec_c::type_e::A_VSELP));
  EXPECT_TRUE(codec_c::look_up("lpcJ").is(codec_c::type_e::A_VSELP));

  EXPECT_TRUE(codec_c::look_up("A_REAL/28_8").is(codec_c::type_e::A_LD_CELP));
  EXPECT_TRUE(codec_c::look_up("28_8").is(codec_c::type_e::A_LD_CELP));
}

TEST(Codec, LookUpStringVideo) {
  EXPECT_TRUE(codec_c::look_up(MKV_V_MPEG1).is(codec_c::type_e::V_MPEG12));
  EXPECT_TRUE(codec_c::look_up(MKV_V_MPEG2).is(codec_c::type_e::V_MPEG12));
  EXPECT_TRUE(codec_c::look_up("h262").is(codec_c::type_e::V_MPEG12));
  EXPECT_TRUE(codec_c::look_up("mp1v").is(codec_c::type_e::V_MPEG12));
  EXPECT_TRUE(codec_c::look_up("mp2v").is(codec_c::type_e::V_MPEG12));
  EXPECT_TRUE(codec_c::look_up("mpeg").is(codec_c::type_e::V_MPEG12));
  EXPECT_TRUE(codec_c::look_up("mpg2").is(codec_c::type_e::V_MPEG12));
  EXPECT_TRUE(codec_c::look_up("mpgv").is(codec_c::type_e::V_MPEG12));

  EXPECT_TRUE(codec_c::look_up(MKV_V_MPEG4_SP).is(codec_c::type_e::V_MPEG4_P2));
  EXPECT_TRUE(codec_c::look_up(MKV_V_MPEG4_ASP).is(codec_c::type_e::V_MPEG4_P2));
  EXPECT_TRUE(codec_c::look_up(MKV_V_MPEG4_AP).is(codec_c::type_e::V_MPEG4_P2));
  EXPECT_TRUE(codec_c::look_up("3iv2").is(codec_c::type_e::V_MPEG4_P2));
  EXPECT_TRUE(codec_c::look_up("divx").is(codec_c::type_e::V_MPEG4_P2));
  EXPECT_TRUE(codec_c::look_up("dx50").is(codec_c::type_e::V_MPEG4_P2));
  EXPECT_TRUE(codec_c::look_up("fmp4").is(codec_c::type_e::V_MPEG4_P2));
  EXPECT_TRUE(codec_c::look_up("mp4v").is(codec_c::type_e::V_MPEG4_P2));
  EXPECT_TRUE(codec_c::look_up("xvid").is(codec_c::type_e::V_MPEG4_P2));
  EXPECT_TRUE(codec_c::look_up("xvix").is(codec_c::type_e::V_MPEG4_P2));

  EXPECT_TRUE(codec_c::look_up(MKV_V_MPEG4_AVC).is(codec_c::type_e::V_MPEG4_P10));
  EXPECT_TRUE(codec_c::look_up("h264").is(codec_c::type_e::V_MPEG4_P10));
  EXPECT_TRUE(codec_c::look_up("x264").is(codec_c::type_e::V_MPEG4_P10));
  EXPECT_TRUE(codec_c::look_up("avc1").is(codec_c::type_e::V_MPEG4_P10));

  EXPECT_TRUE(codec_c::look_up(MKV_V_MPEGH_HEVC).is(codec_c::type_e::V_MPEGH_P2));
  EXPECT_TRUE(codec_c::look_up("h265").is(codec_c::type_e::V_MPEGH_P2));
  EXPECT_TRUE(codec_c::look_up("x265").is(codec_c::type_e::V_MPEGH_P2));
  EXPECT_TRUE(codec_c::look_up("hevc").is(codec_c::type_e::V_MPEGH_P2));

  EXPECT_TRUE(codec_c::look_up(MKV_V_REALV1).is(codec_c::type_e::V_REAL));
  EXPECT_TRUE(codec_c::look_up(MKV_V_REALV2).is(codec_c::type_e::V_REAL));
  EXPECT_TRUE(codec_c::look_up(MKV_V_REALV3).is(codec_c::type_e::V_REAL));
  EXPECT_TRUE(codec_c::look_up(MKV_V_REALV4).is(codec_c::type_e::V_REAL));
  EXPECT_TRUE(codec_c::look_up("rv10").is(codec_c::type_e::V_REAL));
  EXPECT_TRUE(codec_c::look_up("rv20").is(codec_c::type_e::V_REAL));
  EXPECT_TRUE(codec_c::look_up("rv30").is(codec_c::type_e::V_REAL));
  EXPECT_TRUE(codec_c::look_up("rv40").is(codec_c::type_e::V_REAL));

  EXPECT_TRUE(codec_c::look_up(MKV_V_THEORA).is(codec_c::type_e::V_THEORA));
  EXPECT_TRUE(codec_c::look_up("theo").is(codec_c::type_e::V_THEORA));
  EXPECT_TRUE(codec_c::look_up("thra").is(codec_c::type_e::V_THEORA));

  EXPECT_TRUE(codec_c::look_up(MKV_V_DIRAC).is(codec_c::type_e::V_DIRAC));
  EXPECT_TRUE(codec_c::look_up("drac").is(codec_c::type_e::V_DIRAC));

  EXPECT_TRUE(codec_c::look_up(MKV_V_VP8).is(codec_c::type_e::V_VP8));
  EXPECT_TRUE(codec_c::look_up("vp80").is(codec_c::type_e::V_VP8));

  EXPECT_TRUE(codec_c::look_up(MKV_V_VP9).is(codec_c::type_e::V_VP9));
  EXPECT_TRUE(codec_c::look_up("vp90").is(codec_c::type_e::V_VP9));

  EXPECT_TRUE(codec_c::look_up("svq1").is(codec_c::type_e::V_SVQ1));
  EXPECT_TRUE(codec_c::look_up("svq1").is(codec_c::type_e::V_SVQ1));
  EXPECT_TRUE(codec_c::look_up("svqi").is(codec_c::type_e::V_SVQ1));

  EXPECT_TRUE(codec_c::look_up("svq3").is(codec_c::type_e::V_SVQ3));

  EXPECT_TRUE(codec_c::look_up("vc-1").is(codec_c::type_e::V_VC1));
  EXPECT_TRUE(codec_c::look_up("wvc1").is(codec_c::type_e::V_VC1));
}

TEST(Codec, LookUpStringSubtitles) {
  EXPECT_TRUE(codec_c::look_up(MKV_S_TEXTUTF8).is(codec_c::type_e::S_SRT));
  EXPECT_TRUE(codec_c::look_up(MKV_S_TEXTASCII).is(codec_c::type_e::S_SRT));

  EXPECT_TRUE(codec_c::look_up(MKV_S_TEXTSSA).is(codec_c::type_e::S_SSA_ASS));
  EXPECT_TRUE(codec_c::look_up(MKV_S_TEXTASS).is(codec_c::type_e::S_SSA_ASS));
  EXPECT_TRUE(codec_c::look_up("ssa ").is(codec_c::type_e::S_SSA_ASS));
  EXPECT_TRUE(codec_c::look_up("ass ").is(codec_c::type_e::S_SSA_ASS));

  EXPECT_TRUE(codec_c::look_up(MKV_S_TEXTUSF).is(codec_c::type_e::S_USF));
  EXPECT_TRUE(codec_c::look_up("usf ").is(codec_c::type_e::S_USF));

  EXPECT_TRUE(codec_c::look_up(MKV_S_VOBSUB).is(codec_c::type_e::S_VOBSUB));
  EXPECT_TRUE(codec_c::look_up(MKV_S_VOBSUBZLIB).is(codec_c::type_e::S_VOBSUB));

  EXPECT_TRUE(codec_c::look_up(MKV_S_KATE).is(codec_c::type_e::S_KATE));
  EXPECT_TRUE(codec_c::look_up("kate").is(codec_c::type_e::S_KATE));

  EXPECT_TRUE(codec_c::look_up(MKV_S_HDMV_TEXTST).is(codec_c::type_e::S_HDMV_TEXTST));

  EXPECT_TRUE(codec_c::look_up(MKV_S_HDMV_PGS).is(codec_c::type_e::S_HDMV_PGS));
}

TEST(Codec, LookUpAudioFormat) {
  EXPECT_TRUE(codec_c::look_up_audio_format(0x00ffu).is(codec_c::type_e::A_AAC));
  EXPECT_TRUE(codec_c::look_up_audio_format(0x706du).is(codec_c::type_e::A_AAC));
  EXPECT_TRUE(codec_c::look_up_audio_format(0x2000u).is(codec_c::type_e::A_AC3));
  EXPECT_TRUE(codec_c::look_up_audio_format(0x2001u).is(codec_c::type_e::A_DTS));
  EXPECT_TRUE(codec_c::look_up_audio_format(0x0050u).is(codec_c::type_e::A_MP2));
  EXPECT_TRUE(codec_c::look_up_audio_format(0x0055u).is(codec_c::type_e::A_MP3));
  EXPECT_TRUE(codec_c::look_up_audio_format(0x0001u).is(codec_c::type_e::A_PCM));
  EXPECT_TRUE(codec_c::look_up_audio_format(0x0003u).is(codec_c::type_e::A_PCM));
  EXPECT_TRUE(codec_c::look_up_audio_format(0x566fu).is(codec_c::type_e::A_VORBIS));
  EXPECT_TRUE(codec_c::look_up_audio_format(0xfffeu).is(codec_c::type_e::A_VORBIS));
}

TEST(Codec, LookUpFourCC) {
  EXPECT_TRUE(codec_c::look_up(fourcc_c{0x00000000u}).is(codec_c::type_e::V_UNCOMPRESSED));
  EXPECT_TRUE(codec_c::look_up(fourcc_c{0x01000000u}).is(codec_c::type_e::V_RLE8));
  EXPECT_TRUE(codec_c::look_up(fourcc_c{0x02000000u}).is(codec_c::type_e::V_RLE4));
  EXPECT_TRUE(codec_c::look_up(fourcc_c{0x03000000u}).is(codec_c::type_e::V_BITFIELDS));
}

TEST(Codec, LookUpObjectTypeId) {
  EXPECT_TRUE(codec_c::look_up_object_type_id(MP4OTI_MPEG2AudioMain).is(codec_c::type_e::A_AAC));
  EXPECT_TRUE(codec_c::look_up_object_type_id(MP4OTI_MPEG2AudioLowComplexity).is(codec_c::type_e::A_AAC));
  EXPECT_TRUE(codec_c::look_up_object_type_id(MP4OTI_MPEG2AudioScaleableSamplingRate).is(codec_c::type_e::A_AAC));
  EXPECT_TRUE(codec_c::look_up_object_type_id(MP4OTI_MPEG2AudioPart3).is(codec_c::type_e::A_MP3));

  EXPECT_TRUE(codec_c::look_up_object_type_id(MP4OTI_MPEG1Audio).is(codec_c::type_e::A_MP2));

  EXPECT_TRUE(codec_c::look_up_object_type_id(MP4OTI_DTS).is(codec_c::type_e::A_DTS));

  EXPECT_TRUE(codec_c::look_up_object_type_id(MP4OTI_MPEG4Visual).is(codec_c::type_e::V_MPEG4_P2));

  EXPECT_TRUE(codec_c::look_up_object_type_id(MP4OTI_MPEG2VisualSimple).is(codec_c::type_e::V_MPEG12));
  EXPECT_TRUE(codec_c::look_up_object_type_id(MP4OTI_MPEG2VisualMain).is(codec_c::type_e::V_MPEG12));
  EXPECT_TRUE(codec_c::look_up_object_type_id(MP4OTI_MPEG2VisualSNR).is(codec_c::type_e::V_MPEG12));
  EXPECT_TRUE(codec_c::look_up_object_type_id(MP4OTI_MPEG2VisualSpatial).is(codec_c::type_e::V_MPEG12));
  EXPECT_TRUE(codec_c::look_up_object_type_id(MP4OTI_MPEG2VisualHigh).is(codec_c::type_e::V_MPEG12));
  EXPECT_TRUE(codec_c::look_up_object_type_id(MP4OTI_MPEG2Visual422).is(codec_c::type_e::V_MPEG12));
  EXPECT_TRUE(codec_c::look_up_object_type_id(MP4OTI_MPEG1Visual).is(codec_c::type_e::V_MPEG12));

  EXPECT_TRUE(codec_c::look_up_object_type_id(MP4OTI_VOBSUB).is(codec_c::type_e::S_VOBSUB));
}

TEST(Codec, LookUpOverloading) {
  EXPECT_TRUE(codec_c::look_up("kate").is(codec_c::type_e::S_KATE));
  EXPECT_TRUE(codec_c::look_up(std::string{"kate"}).is(codec_c::type_e::S_KATE));
  EXPECT_TRUE(codec_c::look_up(fourcc_c{"kate"}).is(codec_c::type_e::S_KATE));
  EXPECT_TRUE(codec_c::look_up(codec_c::type_e::S_KATE).is(codec_c::type_e::S_KATE));
}

TEST(Codec, LookUpValidity) {
  EXPECT_FALSE(codec_c::look_up("DOES-NOT-EXIST").valid());
  EXPECT_FALSE(codec_c::look_up_audio_format(0x0000u).valid());
  EXPECT_FALSE(codec_c::look_up_audio_format(0x4711).valid());
  EXPECT_FALSE(codec_c::look_up_object_type_id(0x0000u).valid());
  EXPECT_FALSE(codec_c::look_up_object_type_id(0x4711).valid());
}

TEST(Codec, HandlingOfUnknownCodec) {
  EXPECT_TRUE(codec_c::look_up(codec_c::type_e::UNKNOWN).is(codec_c::type_e::UNKNOWN));
  EXPECT_FALSE(codec_c::look_up(codec_c::type_e::UNKNOWN).valid());
}

TEST(Codec, GetNameFallbacks) {
  EXPECT_EQ(codec_c::look_up("DOES-NOT-EXIST").get_name("HelloWorld"), "HelloWorld");
  EXPECT_EQ(codec_c::look_up("DOES-NOT-EXIST").get_name(),             "");
}

TEST(Codec, TrackTypes) {
  EXPECT_EQ(codec_c::look_up(MKV_A_AAC).get_track_type(),    track_audio);
  EXPECT_EQ(codec_c::look_up(MKV_V_VP9).get_track_type(),    track_video);
  EXPECT_EQ(codec_c::look_up(MKV_S_VOBSUB).get_track_type(), track_subtitle);
  EXPECT_EQ(codec_c::look_up(MKV_B_VOBBTN).get_track_type(), track_buttons);
}

}
