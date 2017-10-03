/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Definitions for the various Codec IDs

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include <ostream>

#include "common/container.h"
#include "common/fourcc.h"
#include "matroska/c/libmatroska_t.h"

// see http://www.matroska.org/technical/specs/codecid/index.html

#define MKV_A_AAC         "A_AAC"
#define MKV_A_AAC_2LC     "A_AAC/MPEG2/LC"
#define MKV_A_AAC_2MAIN   "A_AAC/MPEG2/MAIN"
#define MKV_A_AAC_2SBR    "A_AAC/MPEG2/LC/SBR"
#define MKV_A_AAC_2SSR    "A_AAC/MPEG2/SSR"
#define MKV_A_AAC_4LC     "A_AAC/MPEG4/LC"
#define MKV_A_AAC_4LTP    "A_AAC/MPEG4/LTP"
#define MKV_A_AAC_4MAIN   "A_AAC/MPEG4/MAIN"
#define MKV_A_AAC_4SBR    "A_AAC/MPEG4/LC/SBR"
#define MKV_A_AAC_4SSR    "A_AAC/MPEG4/SSR"
#define MKV_A_AC3         "A_AC3"
#define MKV_A_ACM         "A_MS/ACM"
#define MKV_A_ALAC        "A_ALAC"
#define MKV_A_DTS         "A_DTS"
#define MKV_A_EAC3        "A_EAC3"
#define MKV_A_FLAC        "A_FLAC"
#define MKV_A_MLP         "A_MLP"
#define MKV_A_MP2         "A_MPEG/L2"
#define MKV_A_MP3         "A_MPEG/L3"
#define MKV_A_OPUS        "A_OPUS"
#define MKV_A_PCM         "A_PCM/INT/LIT"
#define MKV_A_PCM_BE      "A_PCM/INT/BIG"
#define MKV_A_PCM_FLOAT   "A_PCM/FLOAT/IEEE"
#define MKV_A_QDMC        "A_QUICKTIME/QDMC"
#define MKV_A_QDMC2       "A_QUICKTIME/QDM2"
#define MKV_A_QUICKTIME   "A_QUICKTIME"
#define MKV_A_TRUEHD      "A_TRUEHD"
#define MKV_A_TTA         "A_TTA1"
#define MKV_A_VORBIS      "A_VORBIS"
#define MKV_A_WAVPACK4    "A_WAVPACK4"

#define MKV_V_DIRAC       "V_DIRAC"
#define MKV_V_MPEG1       "V_MPEG1"
#define MKV_V_MPEG2       "V_MPEG2"
#define MKV_V_MPEG4_AP    "V_MPEG4/ISO/AP"
#define MKV_V_MPEG4_ASP   "V_MPEG4/ISO/ASP"
#define MKV_V_MPEG4_AVC   "V_MPEG4/ISO/AVC"
#define MKV_V_MPEG4_SP    "V_MPEG4/ISO/SP"
#define MKV_V_MPEGH_HEVC  "V_MPEGH/ISO/HEVC"
#define MKV_V_MSCOMP      "V_MS/VFW/FOURCC"
#define MKV_V_PRORES      "V_PRORES"
#define MKV_V_QUICKTIME   "V_QUICKTIME"
#define MKV_V_REALV1      "V_REAL/RV10"
#define MKV_V_REALV2      "V_REAL/RV20"
#define MKV_V_REALV3      "V_REAL/RV30"
#define MKV_V_REALV4      "V_REAL/RV40"
#define MKV_V_THEORA      "V_THEORA"
#define MKV_V_VP8         "V_VP8"
#define MKV_V_VP9         "V_VP9"

#define MKV_S_DVBSUB      "S_DVBSUB"
#define MKV_S_HDMV_PGS    "S_HDMV/PGS"
#define MKV_S_HDMV_TEXTST "S_HDMV/TEXTST"
#define MKV_S_KATE        "S_KATE"
#define MKV_S_TEXTASCII   "S_TEXT/ASCII"
#define MKV_S_TEXTASS     "S_TEXT/ASS"
#define MKV_S_TEXTSSA     "S_TEXT/SSA"
#define MKV_S_TEXTUSF     "S_TEXT/USF"
#define MKV_S_TEXTUTF8    "S_TEXT/UTF8"
#define MKV_S_TEXTWEBVTT  "S_TEXT/WEBVTT"
#define MKV_S_VOBSUB      "S_VOBSUB"
#define MKV_S_VOBSUBZLIB  "S_VOBSUB/ZLIB"

#define MKV_B_VOBBTN      "B_VOBBTN"

class codec_c {
public:
  enum class type_e {
      UNKNOWN  = 0
    , V_BITFIELDS = 0x1000
    , V_CINEPAK
    , V_DIRAC
    , V_MPEG12
    , V_MPEG4_P10
    , V_MPEG4_P2
    , V_MPEGH_P2
    , V_PRORES
    , V_REAL
    , V_RLE4
    , V_RLE8
    , V_SVQ1
    , V_SVQ3
    , V_THEORA
    , V_UNCOMPRESSED
    , V_VC1
    , V_VP8
    , V_VP9

    , A_AAC = 0x2000
    , A_AC3
    , A_ACELP_NET
    , A_ALAC
    , A_ATRAC3
    , A_COOK
    , A_DTS
    , A_FLAC
    , A_LD_CELP
    , A_MLP
    , A_MP2
    , A_MP3
    , A_OPUS
    , A_PCM
    , A_QDMC
    , A_RALF
    , A_TRUEHD
    , A_TTA
    , A_VORBIS
    , A_VSELP
    , A_WAVPACK4

    , S_DVBSUB = 0x3000
    , S_HDMV_PGS
    , S_HDMV_TEXTST
    , S_KATE
    , S_SRT
    , S_SSA_ASS
    , S_USF
    , S_VOBSUB
    , S_WEBVTT

    , B_VOBBTN = 0x4000
  };

  enum class specialization_e {
      none

    , dts_hd_master_audio
    , dts_hd_high_resolution
    , dts_express
    , dts_es
    , dts_96_24
    , dts_x

    , mpeg_1_2_layer_1
    , mpeg_1_2_layer_2
    , mpeg_1_2_layer_3

    , truehd_atmos

    , e_ac_3
  };

private:
  using specialization_map_t = std::unordered_map<specialization_e, std::string, mtx::hash<specialization_e> >;

  static std::vector<codec_c> ms_codecs;
  static specialization_map_t ms_specialization_descriptions;

protected:
  boost::regex m_match_re;
  std::string m_name;
  type_e m_type{type_e::UNKNOWN};
  specialization_e m_specialization{specialization_e::none};
  track_type m_track_type{static_cast<track_type>(0)};
  std::vector<uint16_t> m_audio_formats;
  std::vector<fourcc_c> m_fourccs;

public:
  codec_c()
  {
  }

  codec_c(std::string const &name, type_e type, track_type p_track_type, std::string const &match_re, uint16_t audio_format = 0u)
    : m_match_re{(boost::format("(?:%1%)") % match_re).str(), boost::regex::perl | boost::regex::icase}
    , m_name{name}
    , m_type{type}
    , m_track_type{p_track_type}
  {
    if (audio_format)
      m_audio_formats.push_back(audio_format);
  }

  codec_c(std::string const &name, type_e type, track_type p_track_type, std::string const &match_re, fourcc_c const &fourcc)
    : m_match_re{(boost::format("(?:%1%)") % match_re).str(), boost::regex::perl | boost::regex::icase}
    , m_name{name}
    , m_type{type}
    , m_track_type{p_track_type}
    , m_fourccs{fourcc}
  {
  }

  codec_c(std::string const &name, type_e type, track_type p_track_type, std::string const &match_re, std::vector<uint16_t> audio_formats)
    : m_match_re{(boost::format("(?:%1%)") % match_re).str(), boost::regex::perl | boost::regex::icase}
    , m_name{name}
    , m_type{type}
    , m_track_type{p_track_type}
    , m_audio_formats{audio_formats}
  {
  }

  codec_c(std::string const &name, type_e type, track_type p_track_type, std::string const &match_re, std::vector<fourcc_c> fourccs)
    : m_match_re{(boost::format("(?:%1%)") % match_re).str(), boost::regex::perl | boost::regex::icase}
    , m_name{name}
    , m_type{type}
    , m_track_type{p_track_type}
    , m_fourccs{fourccs}
  {
  }

  bool matches(std::string const &fourcc_or_codec_id) const;

  bool valid() const {
    return m_type != type_e::UNKNOWN;
  }

  operator bool() const {
    return valid();
  }

  bool is(type_e type) const {
    return type == m_type;
  }

  std::string const get_name(std::string fallback = "") const;

  type_e get_type() const {
    return m_type;
  }

  specialization_e get_specialization() const {
    return m_specialization;
  }

  track_type get_track_type() const {
    return m_track_type;
  }

  codec_c &set_specialization(specialization_e specialization) {
    m_specialization = specialization;
    return *this;
  }

  codec_c specialize(specialization_e specialization) const {
    auto new_codec = *this;
    new_codec.m_specialization = specialization;
    return new_codec;
  }

private:
  static void initialize();

public:                         // static
  static codec_c const look_up(std::string const &fourcc_or_codec_id);
  static codec_c const look_up(char const *fourcc_or_codec_id);
  static codec_c const look_up(fourcc_c const &fourcc);
  static codec_c const look_up(type_e type);
  static codec_c const look_up_audio_format(uint16_t audio_format);
  static codec_c const look_up_object_type_id(unsigned int object_type_id);

  static std::string const get_name(std::string const &fourcc_or_codec_id, std::string const &fallback);
  static std::string const get_name(type_e type, std::string const &fallback);
};

inline std::ostream &
operator <<(std::ostream &out,
            codec_c const &codec) {
  if (codec)
    out << (boost::format("%1% (0x%|2$04x|)") % codec.get_name() % static_cast<unsigned int>(codec.get_type()));
  else
    out << "<invalid-codec>";

  return out;
}
