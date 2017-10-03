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
codec_c::specialization_map_t codec_c::ms_specialization_descriptions;

void
codec_c::initialize() {
  if (!ms_codecs.empty())
    return;
  ms_codecs.emplace_back("Bitfields",               type_e::V_BITFIELDS,    track_video,    "", fourcc_c{0x03000000u});
  ms_codecs.emplace_back("Cinepak",                 type_e::V_CINEPAK,      track_video,    "cvid");
  ms_codecs.emplace_back("Dirac",                   type_e::V_DIRAC,        track_video,    "drac|V_DIRAC");
  ms_codecs.emplace_back("MPEG-1/2",                type_e::V_MPEG12,       track_video,    "mpeg|mpg[12]|m[12]v.|mpgv|mp[12]v|h262|V_MPEG[12]");
  ms_codecs.emplace_back("MPEG-4p10/AVC/h.264",     type_e::V_MPEG4_P10,    track_video,    "avc.|[hx]264|V_MPEG4/ISO/AVC");
  ms_codecs.emplace_back("MPEG-4p2",                type_e::V_MPEG4_P2,     track_video,    "3iv2|xvi[dx]|divx|dx50|fmp4|mp4v|V_MPEG4/ISO/(?:SP|AP|ASP)");
  ms_codecs.emplace_back("MPEG-H/HEVC/h.265",       type_e::V_MPEGH_P2,     track_video,    "hevc|hvc1|hev1|[hx]265|V_MPEGH/ISO/HEVC");
  ms_codecs.emplace_back("ProRes",                  type_e::V_PRORES,       track_video,    "apch|apcn|apcs|apco|ap4h|V_PRORES");
  ms_codecs.emplace_back("RLE4",                    type_e::V_RLE4,         track_video,    "", fourcc_c{0x02000000u});
  ms_codecs.emplace_back("RLE8",                    type_e::V_RLE8,         track_video,    "", fourcc_c{0x01000000u});
  ms_codecs.emplace_back("RealVideo",               type_e::V_REAL,         track_video,    "rv[1234]\\d|V_REAL/RV\\d+");
  ms_codecs.emplace_back("Sorenson v1",             type_e::V_SVQ1,         track_video,    "svq[i1]");
  ms_codecs.emplace_back("Sorenson v3",             type_e::V_SVQ3,         track_video,    "svq3");
  ms_codecs.emplace_back("Theora",                  type_e::V_THEORA,       track_video,    "theo|thra|V_THEORA");
  ms_codecs.emplace_back("Uncompressed",            type_e::V_UNCOMPRESSED, track_video,    "", fourcc_c{0x00000000u});
  ms_codecs.emplace_back("VC-1",                    type_e::V_VC1,          track_video,    "wvc1|vc-1");
  ms_codecs.emplace_back("VP8",                     type_e::V_VP8,          track_video,    "vp8\\d|V_VP8");
  ms_codecs.emplace_back("VP9",                     type_e::V_VP9,          track_video,    "vp9\\d|V_VP9");

  ms_codecs.emplace_back("AAC",                     type_e::A_AAC,          track_audio,    "mp4a|aac.|raac|racp|A_AAC.*",           std::vector<uint16_t>{ 0x00ffu, 0x706du });
  ms_codecs.emplace_back("AC-3",                    type_e::A_AC3,          track_audio,    "ac3.|ac-3|sac3|eac3|ec-3|a52[\\sb]|dnet|A_E?AC3", 0x2000u);
  ms_codecs.emplace_back("ALAC",                    type_e::A_ALAC,         track_audio,    "alac|A_ALAC");
  ms_codecs.emplace_back("ATRAC3",                  type_e::A_ATRAC3,       track_audio,    "atrc|A_REAL/ATRC");
  ms_codecs.emplace_back("DTS",                     type_e::A_DTS,          track_audio,    "dts[\\sbcehl]|A_DTS",                   0x2001u);
  ms_codecs.emplace_back("FLAC",                    type_e::A_FLAC,         track_audio,    "flac|A_FLAC");
  ms_codecs.emplace_back("G2/Cook",                 type_e::A_COOK,         track_audio,    "cook|A_REAL/COOK");
  ms_codecs.emplace_back("LD-CELP",                 type_e::A_LD_CELP,      track_audio,    "28_8|A_REAL/28_8");
  ms_codecs.emplace_back("MLP",                     type_e::A_MLP,          track_audio,    "mlp\\s|A_MLP");
  ms_codecs.emplace_back("MP2",                     type_e::A_MP2,          track_audio,    "mp2.|\\.mp[12]|mp2a|A_MPEG/L2",         0x0050);
  ms_codecs.emplace_back("MP3",                     type_e::A_MP3,          track_audio,    "mp3.|\\.mp3|LAME|mpga|A_MPEG/L3",       0x0055);
  ms_codecs.emplace_back("Opus",                    type_e::A_OPUS,         track_audio,    "opus|A_OPUS(?:/EXPERIMENTAL)?");
  ms_codecs.emplace_back("PCM",                     type_e::A_PCM,          track_audio,    "twos|sowt|raw.|lpcm|A_PCM/(?:INT|FLOAT)/.+", std::vector<uint16_t>{ 0x0001u, 0x0003u });
  ms_codecs.emplace_back("QDMC",                    type_e::A_QDMC,         track_audio,    "qdm2|A_QUICKTIME/QDM[2C]");
  ms_codecs.emplace_back("RealAudio-Lossless",      type_e::A_RALF,         track_audio,    "ralf|A_REAL/RALF");
  ms_codecs.emplace_back("Sipro/ACELP-NET",         type_e::A_ACELP_NET,    track_audio,    "sipr|A_REAL/SIPR");
  ms_codecs.emplace_back("TrueAudio",               type_e::A_TTA,          track_audio,    "tta1|A_TTA1?");
  ms_codecs.emplace_back("TrueHD",                  type_e::A_TRUEHD,       track_audio,    "trhd|A_TRUEHD");
  ms_codecs.emplace_back("VSELP",                   type_e::A_VSELP,        track_audio,    "lpcj|14_4|A_REAL/LPCJ|A_REAL/14_4");
  ms_codecs.emplace_back("Vorbis",                  type_e::A_VORBIS,       track_audio,    "vor[1b]|A_VORBIS",                      std::vector<uint16_t>{ 0x566fu, 0xfffeu });
  ms_codecs.emplace_back("WavPack4",                type_e::A_WAVPACK4,     track_audio,    "wvpk|A_WAVPACK4");

  ms_codecs.emplace_back("DVBSUB",                  type_e::S_DVBSUB,       track_subtitle, MKV_S_DVBSUB);
  ms_codecs.emplace_back("HDMV PGS",                type_e::S_HDMV_PGS,     track_subtitle, MKV_S_HDMV_PGS);
  ms_codecs.emplace_back("HDMV TextST",             type_e::S_HDMV_TEXTST,  track_subtitle, MKV_S_HDMV_TEXTST);
  ms_codecs.emplace_back("Kate",                    type_e::S_KATE,         track_subtitle, "kate|S_KATE");
  ms_codecs.emplace_back("SubRip/SRT",              type_e::S_SRT,          track_subtitle, "S_TEXT/(?:UTF8|ASCII)");
  ms_codecs.emplace_back("SubStationAlpha",         type_e::S_SSA_ASS,      track_subtitle, "ssa\\s|ass\\s|S_TEXT/(?:SSA|ASS)");
  ms_codecs.emplace_back("UniversalSubtitleFormat", type_e::S_USF,          track_subtitle, "usf\\s|S_TEXT/USF");
  ms_codecs.emplace_back("VobSub",                  type_e::S_VOBSUB,       track_subtitle, "S_VOBSUB(?:/ZLIB)?");
  ms_codecs.emplace_back("WebVTT",                  type_e::S_WEBVTT,       track_subtitle, MKV_S_TEXTWEBVTT);

  ms_codecs.emplace_back("VobButton",               type_e::B_VOBBTN,       track_buttons,  "B_VOBBTN");

  ms_specialization_descriptions.emplace(specialization_e::dts_hd_master_audio,    "DTS-HD Master Audio");
  ms_specialization_descriptions.emplace(specialization_e::dts_hd_high_resolution, "DTS-HD High Resolution Audio");
  ms_specialization_descriptions.emplace(specialization_e::dts_express,            "DTS Express");
  ms_specialization_descriptions.emplace(specialization_e::dts_es,                 "DTS-ES");
  ms_specialization_descriptions.emplace(specialization_e::dts_96_24,              "DTS 96/24");
  ms_specialization_descriptions.emplace(specialization_e::dts_x,                  "DTS:X");

  ms_specialization_descriptions.emplace(specialization_e::mpeg_1_2_layer_1,       "MP1");
  ms_specialization_descriptions.emplace(specialization_e::mpeg_1_2_layer_2,       "MP2");
  ms_specialization_descriptions.emplace(specialization_e::mpeg_1_2_layer_3,       "MP3");

  ms_specialization_descriptions.emplace(specialization_e::truehd_atmos,           "TrueHD Atmos");

  ms_specialization_descriptions.emplace(specialization_e::e_ac_3,                 "E-AC-3");
}

codec_c const
codec_c::look_up(std::string const &fourcc_or_codec_id) {
  initialize();

  auto itr = brng::find_if(ms_codecs, [&fourcc_or_codec_id](codec_c const &c) { return c.matches(fourcc_or_codec_id); });

  return itr == ms_codecs.end() ? codec_c{} : *itr;
}

codec_c const
codec_c::look_up(type_e type) {
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
  initialize();

  auto itr = brng::find_if(ms_codecs, [&fourcc](codec_c const &c) { return brng::find(c.m_fourccs, fourcc) != c.m_fourccs.end(); });

  return itr != ms_codecs.end() ? *itr : look_up(fourcc.str());
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
                    || (MP4OTI_MPEG2AudioScaleableSamplingRate == object_type_id)) ? type_e::A_AAC
                 : MP4OTI_MPEG1Audio                           == object_type_id   ? type_e::A_MP2
                 : MP4OTI_MPEG2AudioPart3                      == object_type_id   ? type_e::A_MP3
                 : MP4OTI_DTS                                  == object_type_id   ? type_e::A_DTS
                 : MP4OTI_VORBIS                               == object_type_id   ? type_e::A_VORBIS
                 : (   (MP4OTI_MPEG2VisualSimple               == object_type_id)
                    || (MP4OTI_MPEG2VisualMain                 == object_type_id)
                    || (MP4OTI_MPEG2VisualSNR                  == object_type_id)
                    || (MP4OTI_MPEG2VisualSpatial              == object_type_id)
                    || (MP4OTI_MPEG2VisualHigh                 == object_type_id)
                    || (MP4OTI_MPEG2Visual422                  == object_type_id)
                    || (MP4OTI_MPEG1Visual                     == object_type_id)) ? type_e::V_MPEG12
                 : MP4OTI_MPEG4Visual                          == object_type_id   ? type_e::V_MPEG4_P2
                 : MP4OTI_VOBSUB                               == object_type_id   ? type_e::S_VOBSUB
                 :                                                                   type_e::UNKNOWN);
}

bool
codec_c::matches(std::string const &fourcc_or_codec_id)
  const {
  if (boost::regex_match(fourcc_or_codec_id, m_match_re))
    return true;

  if (fourcc_or_codec_id.length() == 4)
    return brng::find(m_fourccs, fourcc_c{fourcc_or_codec_id}) != m_fourccs.end();

  return false;
}

std::string const
codec_c::get_name(std::string fallback)
  const {
  if (!valid())
    return fallback;

  if (specialization_e::none == m_specialization)
    return m_name;

  return ms_specialization_descriptions[m_specialization];
}

std::string const
codec_c::get_name(std::string const &fourcc_or_codec_id,
                  std::string const &fallback) {
  auto codec = look_up(fourcc_or_codec_id);
  return codec ? codec.m_name : fallback;
}

std::string const
codec_c::get_name(type_e type,
                  std::string const &fallback) {
  auto codec = look_up(type);
  return codec ? codec.m_name : fallback;
}
