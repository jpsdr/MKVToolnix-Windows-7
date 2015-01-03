/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Definitions for the various Codec IDs

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/codec.h"
#include "common/mp4.h"

std::vector<codec_c> codec_c::ms_codecs;

void
codec_c::initialize() {
  if (!ms_codecs.empty())
    return;
  ms_codecs.emplace_back("MPEG-1/2",                CT_V_MPEG12,    track_video,    "mpeg|mpg[12]|m[12]v.|mpgv|mp[12]v|h262|V_MPEG[12]");
  ms_codecs.emplace_back("MPEG-4p2",                CT_V_MPEG4_P2,  track_video,    "3iv2|xvi[dx]|divx|dx50|fmp4|mp4v|V_MPEG4/ISO/(?:SP|AP|ASP)");
  ms_codecs.emplace_back("MPEG-4p10/AVC/h.264",     CT_V_MPEG4_P10, track_video,    "avc.|[hx]264|V_MPEG4/ISO/AVC");
  ms_codecs.emplace_back("MPEG-H/HEVC/h.265",       CT_V_MPEGH_P2,  track_video,    "hevc|hvc1|hev1|[hx]265|V_MPEGH/ISO/HEVC");
  ms_codecs.emplace_back("RealVideo",               CT_V_REAL,      track_video,    "rv[1234]\\d|V_REAL/RV\\d+");
  ms_codecs.emplace_back("Theora",                  CT_V_THEORA,    track_video,    "theo|thra|V_THEORA");
  ms_codecs.emplace_back("Dirac",                   CT_V_DIRAC,     track_video,    "drac|V_DIRAC");
  ms_codecs.emplace_back("VP8",                     CT_V_VP8,       track_video,    "vp8\\d|V_VP8");
  ms_codecs.emplace_back("VP9",                     CT_V_VP9,       track_video,    "vp9\\d|V_VP9");
  ms_codecs.emplace_back("Sorenson v1",             CT_V_SVQ1,      track_video,    "svq[i1]");
  ms_codecs.emplace_back("Sorenson v3",             CT_V_SVQ3,      track_video,    "svq3");
  ms_codecs.emplace_back("VC1",                     CT_V_VC1,       track_video,    "wvc1|vc-1");

  ms_codecs.emplace_back("AAC",                     CT_A_AAC,       track_audio,    "mp4a|aac.|raac|racp|A_AAC.*",           std::vector<uint16_t>{ 0x00ffu, 0x706du });
  ms_codecs.emplace_back("AC3/EAC3",                CT_A_AC3,       track_audio,    "ac3.|ac-3|sac3|eac3|a52[\\sb]|dnet|A_E?AC3", 0x2000u);
  ms_codecs.emplace_back("ALAC",                    CT_A_ALAC,      track_audio,    "alac|A_ALAC");
  ms_codecs.emplace_back("DTS",                     CT_A_DTS,       track_audio,    "dts[\\sbcehl]|A_DTS",                   0x2001u);
  ms_codecs.emplace_back("MP2",                     CT_A_MP2,       track_audio,    "mp2.|\\.mp[12]|mp2a|A_MPEG/L2",         0x0050);
  ms_codecs.emplace_back("MP3",                     CT_A_MP3,       track_audio,    "mp3.|\\.mp3|LAME|mpga|A_MPEG/L3",       0x0055);
  ms_codecs.emplace_back("PCM",                     CT_A_PCM,       track_audio,    "twos|sowt|A_PCM/(?:INT|FLOAT)/.+",      std::vector<uint16_t>{ 0x0001u, 0x0003u });
  ms_codecs.emplace_back("Vorbis",                  CT_A_VORBIS,    track_audio,    "vor[1b]|A_VORBIS",                      std::vector<uint16_t>{ 0x566fu, 0xfffeu });
  ms_codecs.emplace_back("Opus",                    CT_A_OPUS,      track_audio,    "opus|A_OPUS(?:/EXPERIMENTAL)?");
  ms_codecs.emplace_back("QDMC",                    CT_A_QDMC,      track_audio,    "qdm2|A_QUICKTIME/QDM[2C]");
  ms_codecs.emplace_back("FLAC",                    CT_A_FLAC,      track_audio,    "flac|A_FLAC");
  ms_codecs.emplace_back("MLP",                     CT_A_MLP,       track_audio,    "mlp\\s|A_MLP");
  ms_codecs.emplace_back("TrueHD",                  CT_A_TRUEHD,    track_audio,    "trhd|A_TRUEHD");
  ms_codecs.emplace_back("TrueAudio",               CT_A_TTA,       track_audio,    "tta1|A_TTA1?");
  ms_codecs.emplace_back("WavPack4",                CT_A_WAVPACK4,  track_audio,    "wvpk|A_WAVPACK4");
  ms_codecs.emplace_back("G2/Cook",                 CT_A_COOK,      track_audio,    "cook|A_REAL/COOK");
  ms_codecs.emplace_back("Sipro/ACELP-NET",         CT_A_ACELP_NET, track_audio,    "sipr|A_REAL/SIPR");
  ms_codecs.emplace_back("ATRAC3",                  CT_A_ATRAC3,    track_audio,    "atrc|A_REAL/ATRC");
  ms_codecs.emplace_back("RealAudio-Lossless",      CT_A_RALF,      track_audio,    "ralf|A_REAL/RALF");
  ms_codecs.emplace_back("VSELP",                   CT_A_VSELP,     track_audio,    "lpcj|14_4|A_REAL/LPCJ|A_REAL/14_4");
  ms_codecs.emplace_back("LD-CELP",                 CT_A_LD_CELP,   track_audio,    "28_8|A_REAL/28_8");

  ms_codecs.emplace_back("SubRip/SRT",              CT_S_SRT,       track_subtitle, "S_TEXT/(?:UTF8|ASCII)");
  ms_codecs.emplace_back("SubStationAlpha",         CT_S_SSA_ASS,   track_subtitle, "ssa\\s|ass\\s|S_TEXT/(?:SSA|ASS)");
  ms_codecs.emplace_back("UniversalSubtitleFormat", CT_S_USF,       track_subtitle, "usf\\s|S_TEXT/USF");
  ms_codecs.emplace_back("Kate",                    CT_S_KATE,      track_subtitle, "kate|S_KATE");
  ms_codecs.emplace_back("VobSub",                  CT_S_VOBSUB,    track_subtitle, "S_VOBSUB(?:/ZLIB)?");
  ms_codecs.emplace_back("PGS",                     CT_S_PGS,       track_subtitle, "S_HDMV/PGS");

  ms_codecs.emplace_back("VobButton",               CT_B_VOBBTN,    track_buttons,  "B_VOBBTN");
}

codec_c const
codec_c::look_up(std::string const &fourcc_or_codec_id) {
  initialize();

  auto itr = brng::find_if(ms_codecs, [&fourcc_or_codec_id](codec_c const &c) { return c.matches(fourcc_or_codec_id); });

  return itr == ms_codecs.end() ? codec_c{} : *itr;
}

codec_c const
codec_c::look_up(codec_type_e type) {
  initialize();

  auto itr = brng::find_if(ms_codecs, [type](codec_c const &c) { return c.m_type == type; });

  return itr == ms_codecs.end() ? codec_c{} : *itr;
}

codec_c const
codec_c::look_up(char const *fourcc_or_codec_id) {
  return look_up(std::string{fourcc_or_codec_id});
}

codec_c const
codec_c::look_up(fourcc_c const &fourcc) {
  return look_up(fourcc.str());
}

codec_c const
codec_c::look_up_audio_format(uint16_t audio_format) {
  initialize();

  auto itr = brng::find_if(ms_codecs, [audio_format](codec_c const &c) { return brng::find(c.m_audio_formats, audio_format) != c.m_audio_formats.end(); });

  return itr == ms_codecs.end() ? codec_c{} : *itr;
}

codec_c const
codec_c::look_up_object_type_id(unsigned int object_type_id) {
  return look_up(  (   (MP4OTI_MPEG4Audio                      == object_type_id)
                    || (MP4OTI_MPEG2AudioMain                  == object_type_id)
                    || (MP4OTI_MPEG2AudioLowComplexity         == object_type_id)
                    || (MP4OTI_MPEG2AudioScaleableSamplingRate == object_type_id)) ? CT_A_AAC
                 : MP4OTI_MPEG1Audio                           == object_type_id   ? CT_A_MP2
                 : MP4OTI_MPEG2AudioPart3                      == object_type_id   ? CT_A_MP3
                 : MP4OTI_DTS                                  == object_type_id   ? CT_A_DTS
                 : (   (MP4OTI_MPEG2VisualSimple               == object_type_id)
                    || (MP4OTI_MPEG2VisualMain                 == object_type_id)
                    || (MP4OTI_MPEG2VisualSNR                  == object_type_id)
                    || (MP4OTI_MPEG2VisualSpatial              == object_type_id)
                    || (MP4OTI_MPEG2VisualHigh                 == object_type_id)
                    || (MP4OTI_MPEG2Visual422                  == object_type_id)
                    || (MP4OTI_MPEG1Visual                     == object_type_id)) ? CT_V_MPEG12
                 : MP4OTI_MPEG4Visual                          == object_type_id   ? CT_V_MPEG4_P2
                 : MP4OTI_VOBSUB                               == object_type_id   ? CT_S_VOBSUB
                 :                                                                   CT_UNKNOWN);
}

std::string const
codec_c::get_name(std::string const &fourcc_or_codec_id,
                  std::string const &fallback) {
  auto codec = look_up(fourcc_or_codec_id);
  return codec ? codec.m_name : fallback;
}

std::string const
codec_c::get_name(codec_type_e type,
                  std::string const &fallback) {
  auto codec = look_up(type);
  return codec ? codec.m_name : fallback;
}
