/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Definitions for the various Codec IDs

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <QRegularExpression>

#include "common/codec.h"
#include "common/mp4.h"
#include "common/qt.h"

namespace {

using specialization_map_t = std::unordered_map<codec_c::specialization_e, std::string, mtx::hash<codec_c::specialization_e> >;

std::vector<codec_c> s_codecs;
specialization_map_t s_specialization_descriptions;

}

class codec_private_c {
  friend class codec_c;

protected:
  std::string name;
  codec_c::type_e type{codec_c::type_e::UNKNOWN};
  codec_c::specialization_e specialization{codec_c::specialization_e::none};
  track_type the_track_type{static_cast<track_type>(0)};
  QRegularExpression match_re;
  std::vector<fourcc_c> fourccs;
  std::vector<uint16_t> audio_formats;

public:
  codec_private_c() = default;
  codec_private_c(std::string const &p_name, codec_c::type_e p_type, track_type p_track_type, std::string const &p_match_re);

  codec_private_c(codec_private_c const &src) = default;
  codec_private_c(codec_private_c &&src) = default;

  codec_private_c &operator =(codec_private_c const &src) = default;
  codec_private_c &operator =(codec_private_c &&src) = default;
};

codec_private_c::codec_private_c(std::string const &p_name,
                                 codec_c::type_e p_type,
                                 track_type p_track_type,
                                 std::string const &p_match_re)
  : name{p_name}
  , type{p_type}
  , the_track_type{p_track_type}
  , match_re{Q(fmt::format("^(?:{0})$", p_match_re)), QRegularExpression::CaseInsensitiveOption}
{
}

// ------------------------------------------------------------

codec_c::codec_c()
  : p_ptr{new codec_private_c}
{
}

codec_c::codec_c(codec_private_c &p)
  : p_ptr{&p}
{
}

codec_c::codec_c(std::string const &name,
                 type_e type,
                 track_type p_track_type,
                 std::string const &match_re,
                 uint16_t audio_format)
  : p_ptr{new codec_private_c{name, type, p_track_type, match_re}}
{
  if (audio_format)
    p_ptr->audio_formats.push_back(audio_format);
}

codec_c::codec_c(std::string const &name,
                 type_e type,
                 track_type p_track_type,
                 std::string const &match_re,
                 fourcc_c const &fourcc)
  : p_ptr{new codec_private_c{name, type, p_track_type, match_re}}
{
  p_ptr->fourccs.push_back(fourcc);
}

codec_c::codec_c(std::string const &name,
                 type_e type,
                 track_type p_track_type,
                 std::string const &match_re,
                 std::vector<uint16_t> const &audio_formats)
  : p_ptr{new codec_private_c{name, type, p_track_type, match_re}}
{
  p_ptr->audio_formats = audio_formats;
}

codec_c::codec_c(std::string const &name,
                 type_e type,
                 track_type p_track_type,
                 std::string const &match_re,
                 std::vector<fourcc_c> const &fourccs)
  : p_ptr{new codec_private_c{name, type, p_track_type, match_re}}
{
  p_ptr->fourccs = fourccs;
}

codec_c::codec_c(codec_c const &src)
  : p_ptr{new codec_private_c{*src.p_ptr}}
{
}

codec_c::~codec_c() {
}

codec_c &
codec_c::operator =(codec_c const &src) {
  *p_ptr = *src.p_ptr;
  return *this;
}

void
codec_c::initialize() {
  if (!s_codecs.empty())
    return;
  s_codecs.emplace_back("AV1",                     type_e::V_AV1,          track_video,    "av01|V_AV1", fourcc_c{"AV01"});
  s_codecs.emplace_back("AVC/H.264/MPEG-4p10",     type_e::V_MPEG4_P10,    track_video,    "avc.|[hx]264|V_MPEG4/ISO/AVC");
  s_codecs.emplace_back("Bitfields",               type_e::V_BITFIELDS,    track_video,    "", fourcc_c{0x03000000u});
  s_codecs.emplace_back("Cinepak",                 type_e::V_CINEPAK,      track_video,    "cvid");
  s_codecs.emplace_back("Dirac",                   type_e::V_DIRAC,        track_video,    "drac|V_DIRAC");
  s_codecs.emplace_back("HEVC/H.265/MPEG-H",       type_e::V_MPEGH_P2,     track_video,    "hevc|hvc1|hev1|[hx]265|dvh[1e]|V_MPEGH/ISO/HEVC");
  s_codecs.emplace_back("MPEG-1/2",                type_e::V_MPEG12,       track_video,    "mpeg|mpg[12]|m[12]v.|mpgv|mp[12]v|h262|V_MPEG[12]");
  s_codecs.emplace_back("MPEG-4p2",                type_e::V_MPEG4_P2,     track_video,    "3iv2|xvi[dx]|divx|dx50|fmp4|mp4v|V_MPEG4/ISO/(?:SP|AP|ASP)");
  s_codecs.emplace_back("ProRes",                  type_e::V_PRORES,       track_video,    "apch|apcn|apcs|apco|ap4h|V_PRORES");
  s_codecs.emplace_back("RLE4",                    type_e::V_RLE4,         track_video,    "", fourcc_c{0x02000000u});
  s_codecs.emplace_back("RLE8",                    type_e::V_RLE8,         track_video,    "", fourcc_c{0x01000000u});
  s_codecs.emplace_back("RealVideo",               type_e::V_REAL,         track_video,    "rv[1234]\\d|V_REAL/RV\\d+");
  s_codecs.emplace_back("Sorenson v1",             type_e::V_SVQ1,         track_video,    "svq[i1]");
  s_codecs.emplace_back("Sorenson v3",             type_e::V_SVQ3,         track_video,    "svq3");
  s_codecs.emplace_back("Theora",                  type_e::V_THEORA,       track_video,    "theo|thra|V_THEORA");
  s_codecs.emplace_back("Uncompressed",            type_e::V_UNCOMPRESSED, track_video,    "", fourcc_c{0x00000000u});
  s_codecs.emplace_back("VC-1",                    type_e::V_VC1,          track_video,    "wvc1|vc-1");
  s_codecs.emplace_back("VP8",                     type_e::V_VP8,          track_video,    "vp8\\d|V_VP8", fourcc_c{"VP80"});
  s_codecs.emplace_back("VP9",                     type_e::V_VP9,          track_video,    "vp9\\d|V_VP9", std::vector<fourcc_c>{ fourcc_c{"VP90"}, fourcc_c{"vp09"} });

  s_codecs.emplace_back("AAC",                     type_e::A_AAC,          track_audio,    "mp4a|aac.|raac|racp|A_AAC.*",           std::vector<uint16_t>{ 0x00ffu, 0x706du });
  s_codecs.emplace_back("AC-3",                    type_e::A_AC3,          track_audio,    "ac3.|ac-3|sac3|eac3|ec-3|a52[\\sb]|dnet|A_E?AC3", 0x2000u);
  s_codecs.emplace_back("ALAC",                    type_e::A_ALAC,         track_audio,    "alac|A_ALAC");
  s_codecs.emplace_back("ATRAC3",                  type_e::A_ATRAC3,       track_audio,    "atrc|A_REAL/ATRC");
  s_codecs.emplace_back("DTS",                     type_e::A_DTS,          track_audio,    "dts[\\sbcehl]|A_DTS",                   0x2001u);
  s_codecs.emplace_back("FLAC",                    type_e::A_FLAC,         track_audio,    "flac|A_FLAC");
  s_codecs.emplace_back("G2/Cook",                 type_e::A_COOK,         track_audio,    "cook|A_REAL/COOK");
  s_codecs.emplace_back("LD-CELP",                 type_e::A_LD_CELP,      track_audio,    "28_8|A_REAL/28_8");
  s_codecs.emplace_back("MLP",                     type_e::A_MLP,          track_audio,    "mlp\\s|A_MLP");
  s_codecs.emplace_back("MP2",                     type_e::A_MP2,          track_audio,    "mp2.|\\.mp[12]|mp2a|A_MPEG/L2",         0x0050);
  s_codecs.emplace_back("MP3",                     type_e::A_MP3,          track_audio,    "mp3.|\\.mp3|LAME|mpga|A_MPEG/L3",       0x0055);
  s_codecs.emplace_back("Opus",                    type_e::A_OPUS,         track_audio,    "opus|A_OPUS(?:/EXPERIMENTAL)?");
  s_codecs.emplace_back("PCM",                     type_e::A_PCM,          track_audio,    "twos|sowt|raw.|lpcm|in24|A_PCM/(?:INT|FLOAT)/.+", std::vector<uint16_t>{ 0x0001u, 0x0003u });
  s_codecs.emplace_back("QDMC",                    type_e::A_QDMC,         track_audio,    "qdm2|A_QUICKTIME/QDM[2C]");
  s_codecs.emplace_back("RealAudio-Lossless",      type_e::A_RALF,         track_audio,    "ralf|A_REAL/RALF");
  s_codecs.emplace_back("Sipro/ACELP-NET",         type_e::A_ACELP_NET,    track_audio,    "sipr|A_REAL/SIPR");
  s_codecs.emplace_back("TrueAudio",               type_e::A_TTA,          track_audio,    "tta1|A_TTA1?");
  s_codecs.emplace_back("TrueHD",                  type_e::A_TRUEHD,       track_audio,    "trhd|mlpa|A_TRUEHD");
  s_codecs.emplace_back("VSELP",                   type_e::A_VSELP,        track_audio,    "lpcj|14_4|A_REAL/LPCJ|A_REAL/14_4");
  s_codecs.emplace_back("Vorbis",                  type_e::A_VORBIS,       track_audio,    "vor[1b]|A_VORBIS",                      std::vector<uint16_t>{ 0x566fu, 0xfffeu });
  s_codecs.emplace_back("WavPack4",                type_e::A_WAVPACK4,     track_audio,    "wvpk|A_WAVPACK4");

  s_codecs.emplace_back("DVBSUB",                  type_e::S_DVBSUB,       track_subtitle, MKV_S_DVBSUB);
  s_codecs.emplace_back("HDMV PGS",                type_e::S_HDMV_PGS,     track_subtitle, MKV_S_HDMV_PGS);
  s_codecs.emplace_back("HDMV TextST",             type_e::S_HDMV_TEXTST,  track_subtitle, MKV_S_HDMV_TEXTST);
  s_codecs.emplace_back("Kate",                    type_e::S_KATE,         track_subtitle, "kate|S_KATE");
  s_codecs.emplace_back("SubRip/SRT",              type_e::S_SRT,          track_subtitle, "S_TEXT/(?:UTF8|ASCII)");
  s_codecs.emplace_back("SubStationAlpha",         type_e::S_SSA_ASS,      track_subtitle, "ssa\\s|ass\\s|S_TEXT/(?:SSA|ASS)");
  s_codecs.emplace_back("Timed Text",              type_e::S_TX3G,         track_subtitle, "tx3g");
  s_codecs.emplace_back("UniversalSubtitleFormat", type_e::S_USF,          track_subtitle, "usf\\s|S_TEXT/USF");
  s_codecs.emplace_back("VobSub",                  type_e::S_VOBSUB,       track_subtitle, "S_VOBSUB(?:/ZLIB)?");
  s_codecs.emplace_back("WebVTT",                  type_e::S_WEBVTT,       track_subtitle, MKV_S_TEXTWEBVTT);

  s_codecs.emplace_back("VobButton",               type_e::B_VOBBTN,       track_buttons,  "B_VOBBTN");

  s_specialization_descriptions.emplace(specialization_e::dts_hd_master_audio,    "DTS-HD Master Audio");
  s_specialization_descriptions.emplace(specialization_e::dts_hd_high_resolution, "DTS-HD High Resolution Audio");
  s_specialization_descriptions.emplace(specialization_e::dts_express,            "DTS Express");
  s_specialization_descriptions.emplace(specialization_e::dts_es,                 "DTS-ES");
  s_specialization_descriptions.emplace(specialization_e::dts_96_24,              "DTS 96/24");
  s_specialization_descriptions.emplace(specialization_e::dts_x,                  "DTS:X");

  s_specialization_descriptions.emplace(specialization_e::mpeg_1_2_layer_1,       "MP1");
  s_specialization_descriptions.emplace(specialization_e::mpeg_1_2_layer_2,       "MP2");
  s_specialization_descriptions.emplace(specialization_e::mpeg_1_2_layer_3,       "MP3");

  s_specialization_descriptions.emplace(specialization_e::truehd_atmos,           "TrueHD Atmos");

  s_specialization_descriptions.emplace(specialization_e::ac3_dolby_surround_ex,  "AC-3 Dolby Surround EX");
  s_specialization_descriptions.emplace(specialization_e::e_ac_3,                 "E-AC-3");
}

bool
codec_c::valid()
  const {
  return p_func()->type != type_e::UNKNOWN;
}

codec_c::operator bool()
  const {
  return valid();
}

bool
codec_c::is(type_e type)
  const {
  return type == p_func()->type;
}

codec_c::type_e
codec_c::get_type()
  const {
  return p_func()->type;
}

codec_c::specialization_e
codec_c::get_specialization()
  const {
  return p_func()->specialization;
}

track_type
codec_c::get_track_type()
  const {
  return p_func()->the_track_type;
}

std::vector<fourcc_c> const &
codec_c::get_fourccs()
  const {
  return p_func()->fourccs;
}

std::vector<uint16_t> const &
codec_c::get_audio_formats()
  const {
  return p_func()->audio_formats;
}

codec_c &
codec_c::set_specialization(specialization_e specialization) {
  p_func()->specialization = specialization;
  return *this;
}

codec_c
codec_c::specialize(specialization_e specialization)
  const {
  auto new_codec = *this;
  new_codec.set_specialization(specialization);
  return new_codec;
}

codec_c const
codec_c::look_up(std::string const &fourcc_or_codec_id) {
  initialize();

  auto itr = std::find_if(s_codecs.begin(), s_codecs.end(), [&fourcc_or_codec_id](codec_c const &c) { return c.matches(fourcc_or_codec_id); });

  return itr == s_codecs.end() ? codec_c{} : *itr;
}

codec_c const
codec_c::look_up(type_e type) {
  initialize();

  auto itr = std::find_if(s_codecs.begin(), s_codecs.end(), [type](codec_c const &c) { return c.get_type() == type; });

  return itr == s_codecs.end() ? codec_c{} : *itr;
}

codec_c const
codec_c::look_up(char const *fourcc_or_codec_id) {
  return look_up(std::string{fourcc_or_codec_id});
}

codec_c const
codec_c::look_up(fourcc_c const &fourcc) {
  initialize();

  auto itr = std::find_if(s_codecs.begin(), s_codecs.end(), [&fourcc](codec_c const &c) {
    auto const &fourccs = c.get_fourccs();
    return std::find(fourccs.begin(), fourccs.end(), fourcc) != fourccs.end();
  });

  return itr != s_codecs.end() ? *itr : look_up(fourcc.str());
}

codec_c const
codec_c::look_up_bluray_stream_coding_type(uint8_t coding_type) {
  switch (coding_type) {
    case static_cast<uint8_t>(mtx::bluray::mpls::stream_coding_type_e::mpeg2_video_primary_secondary):      return look_up(type_e::V_MPEG12);
    case static_cast<uint8_t>(mtx::bluray::mpls::stream_coding_type_e::mpeg4_avc_video_primary_secondary):  return look_up(type_e::V_MPEG4_P10);
    case static_cast<uint8_t>(mtx::bluray::mpls::stream_coding_type_e::mpegh_hevc_video_primary_secondary): return look_up(type_e::V_MPEGH_P2);
    case static_cast<uint8_t>(mtx::bluray::mpls::stream_coding_type_e::vc1_video_primary_secondary):        return look_up(type_e::V_VC1);
    case static_cast<uint8_t>(mtx::bluray::mpls::stream_coding_type_e::lpcm_audio_primary):                 return look_up(type_e::A_PCM);
    case static_cast<uint8_t>(mtx::bluray::mpls::stream_coding_type_e::ac3_audio_primary):                  return look_up(type_e::A_AC3);
    case static_cast<uint8_t>(mtx::bluray::mpls::stream_coding_type_e::dts_audio_primary):                  return look_up(type_e::A_DTS);
    case static_cast<uint8_t>(mtx::bluray::mpls::stream_coding_type_e::truehd_audio_primary):               return look_up(type_e::A_TRUEHD);
    case static_cast<uint8_t>(mtx::bluray::mpls::stream_coding_type_e::eac3_audio_primary):                 return look_up(type_e::A_AC3).specialize(specialization_e::e_ac_3);
    case static_cast<uint8_t>(mtx::bluray::mpls::stream_coding_type_e::dts_hd_audio_primary):               return look_up(type_e::A_DTS).specialize(specialization_e::dts_hd_master_audio);
    case static_cast<uint8_t>(mtx::bluray::mpls::stream_coding_type_e::dts_hd_xll_audio_primary):           return look_up(type_e::A_DTS).specialize(specialization_e::dts_x);
    case static_cast<uint8_t>(mtx::bluray::mpls::stream_coding_type_e::eac3_audio_secondary):               return look_up(type_e::A_AC3).specialize(specialization_e::e_ac_3);
    case static_cast<uint8_t>(mtx::bluray::mpls::stream_coding_type_e::dts_hd_audio_secondary):             return look_up(type_e::A_DTS).specialize(specialization_e::dts_hd_master_audio);
    case static_cast<uint8_t>(mtx::bluray::mpls::stream_coding_type_e::presentation_graphics_subtitles):    return look_up(type_e::S_HDMV_PGS);
    case static_cast<uint8_t>(mtx::bluray::mpls::stream_coding_type_e::interactive_graphics_menu):          return {}; // unsupported
    case static_cast<uint8_t>(mtx::bluray::mpls::stream_coding_type_e::text_subtitles):                     return look_up(type_e::S_HDMV_TEXTST);
  }

  return {};
}

codec_c const
codec_c::look_up_audio_format(uint16_t audio_format) {
  initialize();

  auto itr = std::find_if(s_codecs.begin(), s_codecs.end(), [audio_format](codec_c const &c) {
    auto const &audio_formats = c.get_audio_formats();
    return std::find(audio_formats.begin(), audio_formats.end(), audio_format) != audio_formats.end();
  });

  return itr == s_codecs.end() ? codec_c{} : *itr;
}

codec_c const
codec_c::look_up_object_type_id(unsigned int object_type_id) {
  return look_up(  (   (mtx::mp4::OBJECT_TYPE_MPEG4Audio                      == object_type_id)
                    || (mtx::mp4::OBJECT_TYPE_MPEG2AudioMain                  == object_type_id)
                    || (mtx::mp4::OBJECT_TYPE_MPEG2AudioLowComplexity         == object_type_id)
                    || (mtx::mp4::OBJECT_TYPE_MPEG2AudioScaleableSamplingRate == object_type_id)) ? type_e::A_AAC
                 : mtx::mp4::OBJECT_TYPE_MPEG1Audio                           == object_type_id   ? type_e::A_MP2
                 : mtx::mp4::OBJECT_TYPE_MPEG2AudioPart3                      == object_type_id   ? type_e::A_MP3
                 : mtx::mp4::OBJECT_TYPE_DTS                                  == object_type_id   ? type_e::A_DTS
                 : mtx::mp4::OBJECT_TYPE_VORBIS                               == object_type_id   ? type_e::A_VORBIS
                 : (   (mtx::mp4::OBJECT_TYPE_MPEG2VisualSimple               == object_type_id)
                    || (mtx::mp4::OBJECT_TYPE_MPEG2VisualMain                 == object_type_id)
                    || (mtx::mp4::OBJECT_TYPE_MPEG2VisualSNR                  == object_type_id)
                    || (mtx::mp4::OBJECT_TYPE_MPEG2VisualSpatial              == object_type_id)
                    || (mtx::mp4::OBJECT_TYPE_MPEG2VisualHigh                 == object_type_id)
                    || (mtx::mp4::OBJECT_TYPE_MPEG2Visual422                  == object_type_id)
                    || (mtx::mp4::OBJECT_TYPE_MPEG1Visual                     == object_type_id)) ? type_e::V_MPEG12
                 : mtx::mp4::OBJECT_TYPE_MPEG4Visual                          == object_type_id   ? type_e::V_MPEG4_P2
                 : mtx::mp4::OBJECT_TYPE_VOBSUB                               == object_type_id   ? type_e::S_VOBSUB
                 :                                                                                  type_e::UNKNOWN);
}

bool
codec_c::matches(std::string const &fourcc_or_codec_id)
  const {
  auto &p = *p_func();

  if (Q(fourcc_or_codec_id).contains(p.match_re))
    return true;

  if (fourcc_or_codec_id.length() == 4)
    return std::find(p.fourccs.begin(), p.fourccs.end(), fourcc_c{fourcc_or_codec_id}) != p.fourccs.end();

  return false;
}

std::string const
codec_c::get_name(std::string fallback)
  const {
  auto &p = *p_func();

  if (!valid())
    return fallback;

  if (specialization_e::none == p.specialization)
    return p.name;

  return s_specialization_descriptions[p.specialization];
}

std::string const
codec_c::get_name(std::string const &fourcc_or_codec_id,
                  std::string const &fallback) {
  auto codec = look_up(fourcc_or_codec_id);
  return codec ? codec.get_name() : fallback;
}

std::string const
codec_c::get_name(type_e type,
                  std::string const &fallback) {
  auto codec = look_up(type);
  return codec ? codec.get_name() : fallback;
}
