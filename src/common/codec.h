/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Definitions for the various Codec IDs

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/bluray/mpls.h"
#include "common/common_pch.h"

#include <ostream>

#include "common/container.h"
#include "common/fourcc.h"

// see http://www.matroska.org/technical/specs/codecid/index.html

constexpr auto MKV_A_AAC          = "A_AAC";
constexpr auto MKV_A_AAC_2LC      = "A_AAC/MPEG2/LC";
constexpr auto MKV_A_AAC_2MAIN    = "A_AAC/MPEG2/MAIN";
constexpr auto MKV_A_AAC_2SBR     = "A_AAC/MPEG2/LC/SBR";
constexpr auto MKV_A_AAC_2SSR     = "A_AAC/MPEG2/SSR";
constexpr auto MKV_A_AAC_4LC      = "A_AAC/MPEG4/LC";
constexpr auto MKV_A_AAC_4LTP     = "A_AAC/MPEG4/LTP";
constexpr auto MKV_A_AAC_4MAIN    = "A_AAC/MPEG4/MAIN";
constexpr auto MKV_A_AAC_4SBR     = "A_AAC/MPEG4/LC/SBR";
constexpr auto MKV_A_AAC_4SSR     = "A_AAC/MPEG4/SSR";
constexpr auto MKV_A_AC3          = "A_AC3";
constexpr auto MKV_A_ACM          = "A_MS/ACM";
constexpr auto MKV_A_ALAC         = "A_ALAC";
constexpr auto MKV_A_DTS          = "A_DTS";
constexpr auto MKV_A_EAC3         = "A_EAC3";
constexpr auto MKV_A_FLAC         = "A_FLAC";
constexpr auto MKV_A_MLP          = "A_MLP";
constexpr auto MKV_A_MP2          = "A_MPEG/L2";
constexpr auto MKV_A_MP3          = "A_MPEG/L3";
constexpr auto MKV_A_OPUS         = "A_OPUS";
constexpr auto MKV_A_PCM          = "A_PCM/INT/LIT";
constexpr auto MKV_A_PCM_BE       = "A_PCM/INT/BIG";
constexpr auto MKV_A_PCM_FLOAT    = "A_PCM/FLOAT/IEEE";
constexpr auto MKV_A_QDMC         = "A_QUICKTIME/QDMC";
constexpr auto MKV_A_QDMC2        = "A_QUICKTIME/QDM2";
constexpr auto MKV_A_QUICKTIME    = "A_QUICKTIME";
constexpr auto MKV_A_TRUEHD       = "A_TRUEHD";
constexpr auto MKV_A_TTA          = "A_TTA1";
constexpr auto MKV_A_VORBIS       = "A_VORBIS";
constexpr auto MKV_A_WAVPACK4     = "A_WAVPACK4";

constexpr auto MKV_V_AV1          = "V_AV1";
constexpr auto MKV_V_DIRAC        = "V_DIRAC";
constexpr auto MKV_V_MPEG1        = "V_MPEG1";
constexpr auto MKV_V_MPEG2        = "V_MPEG2";
constexpr auto MKV_V_MPEG4_AP     = "V_MPEG4/ISO/AP";
constexpr auto MKV_V_MPEG4_ASP    = "V_MPEG4/ISO/ASP";
constexpr auto MKV_V_MPEG4_AVC    = "V_MPEG4/ISO/AVC";
constexpr auto MKV_V_MPEG4_SP     = "V_MPEG4/ISO/SP";
constexpr auto MKV_V_MPEGH_HEVC   = "V_MPEGH/ISO/HEVC";
constexpr auto MKV_V_MSCOMP       = "V_MS/VFW/FOURCC";
constexpr auto MKV_V_PRORES       = "V_PRORES";
constexpr auto MKV_V_QUICKTIME    = "V_QUICKTIME";
constexpr auto MKV_V_REALV1       = "V_REAL/RV10";
constexpr auto MKV_V_REALV2       = "V_REAL/RV20";
constexpr auto MKV_V_REALV3       = "V_REAL/RV30";
constexpr auto MKV_V_REALV4       = "V_REAL/RV40";
constexpr auto MKV_V_THEORA       = "V_THEORA";
constexpr auto MKV_V_UNCOMPRESSED = "V_UNCOMPRESSED";
constexpr auto MKV_V_VP8          = "V_VP8";
constexpr auto MKV_V_VP9          = "V_VP9";

constexpr auto MKV_S_DVBSUB       = "S_DVBSUB";
constexpr auto MKV_S_HDMV_PGS     = "S_HDMV/PGS";
constexpr auto MKV_S_HDMV_TEXTST  = "S_HDMV/TEXTST";
constexpr auto MKV_S_KATE         = "S_KATE";
constexpr auto MKV_S_TEXTASCII    = "S_TEXT/ASCII";
constexpr auto MKV_S_TEXTASS      = "S_TEXT/ASS";
constexpr auto MKV_S_TEXTSSA      = "S_TEXT/SSA";
constexpr auto MKV_S_TEXTUSF      = "S_TEXT/USF";
constexpr auto MKV_S_TEXTUTF8     = "S_TEXT/UTF8";
constexpr auto MKV_S_TEXTWEBVTT   = "S_TEXT/WEBVTT";
constexpr auto MKV_S_VOBSUB       = "S_VOBSUB";
constexpr auto MKV_S_VOBSUBZLIB   = "S_VOBSUB/ZLIB";

constexpr auto MKV_B_VOBBTN       = "B_VOBBTN";

class codec_private_c;
class codec_c {
protected:
  MTX_DECLARE_PRIVATE(codec_private_c)

  std::unique_ptr<codec_private_c> const p_ptr;

  explicit codec_c(codec_private_c &p);

public:
  enum class type_e {
      UNKNOWN  = 0
    , V_AV1 = 0x1000
    , V_BITFIELDS
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
    , S_TX3G
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

    , ac3_dolby_surround_ex
    , e_ac_3
  };

public:
  codec_c();
  virtual ~codec_c();

  codec_c(std::string const &name, type_e type, track_type p_track_type, std::string const &match_re, uint16_t audio_format = 0u);
  codec_c(std::string const &name, type_e type, track_type p_track_type, std::string const &match_re, fourcc_c const &fourcc);
  codec_c(std::string const &name, type_e type, track_type p_track_type, std::string const &match_re, std::vector<uint16_t> const &audio_formats);
  codec_c(std::string const &name, type_e type, track_type p_track_type, std::string const &match_re, std::vector<fourcc_c> const &fourccs);

  codec_c(codec_c const &src);

  codec_c &operator =(codec_c const &src);

  bool matches(std::string const &fourcc_or_codec_id) const;

  bool valid() const;
  operator bool() const;
  bool is(type_e type) const;

  std::string const get_name(std::string fallback = "") const;
  type_e get_type() const;
  specialization_e get_specialization() const;
  track_type get_track_type() const;
  codec_c &set_specialization(specialization_e specialization);
  codec_c specialize(specialization_e specialization) const;

  std::vector<fourcc_c> const &get_fourccs() const;
  std::vector<uint16_t> const &get_audio_formats() const;

private:
  static void initialize();

public:                         // static
  static codec_c const look_up(std::string const &fourcc_or_codec_id);
  static codec_c const look_up(char const *fourcc_or_codec_id);
  static codec_c const look_up(fourcc_c const &fourcc);
  static codec_c const look_up(type_e type);
  static codec_c const look_up_bluray_stream_coding_type(uint8_t coding_type);
  static codec_c const look_up_audio_format(uint16_t audio_format);
  static codec_c const look_up_object_type_id(unsigned int object_type_id);

  static std::string const get_name(std::string const &fourcc_or_codec_id, std::string const &fallback);
  static std::string const get_name(type_e type, std::string const &fallback);
};

inline std::ostream &
operator <<(std::ostream &out,
            codec_c const &codec) {
  if (codec)
    out << fmt::format("{0} (0x{1:04x})", codec.get_name(), static_cast<unsigned int>(codec.get_type()));
  else
    out << "<invalid-codec>";

  return out;
}

#if FMT_VERSION >= 90000
template <> struct fmt::formatter<codec_c> : ostream_formatter {};
#endif  // FMT_VERSION >= 90000
