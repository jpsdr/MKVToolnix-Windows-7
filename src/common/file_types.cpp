/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   File type enum

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/file_types.h"

namespace mtx {

static std::vector<file_type_t> s_supported_file_types;

std::vector<file_type_t> &
file_type_t::get_supported() {
  if (!s_supported_file_types.empty())
    return s_supported_file_types;

  s_supported_file_types.push_back(file_type_t(Y("A/52 (aka AC-3)"),                     "ac3 eac3"));
  s_supported_file_types.push_back(file_type_t(Y("AAC (Advanced Audio Coding)"),         "aac m4a mp4"));
  s_supported_file_types.push_back(file_type_t(Y("AVC/h.264 elementary streams"),        "264 avc h264 x264"));
  s_supported_file_types.push_back(file_type_t(Y("AVI (Audio/Video Interleaved)"),       "avi"));
  s_supported_file_types.push_back(file_type_t(Y("ALAC (Apple Lossless Audio Codec)"),   "caf m4a mp4"));
  s_supported_file_types.push_back(file_type_t(Y("Dirac"),                               "drc"));
  s_supported_file_types.push_back(file_type_t(Y("Dolby TrueHD"),                        "thd thd+ac3 truehd true-hd"));
  s_supported_file_types.push_back(file_type_t(Y("DTS/DTS-HD (Digital Theater System)"), "dts dtshd dts-hd"));
#if defined(HAVE_FLAC_FORMAT_H)
  s_supported_file_types.push_back(file_type_t(Y("FLAC (Free Lossless Audio Codec)"),    "flac ogg"));
#endif
  s_supported_file_types.push_back(file_type_t(Y("FLV (Flash Video)"),                   "flv"));
  s_supported_file_types.push_back(file_type_t(Y("HDMV TextST"),                         "textst"));
  s_supported_file_types.push_back(file_type_t(Y("HEVC/h.265 elementary streams"),       "265 hevc h265 x265"));
  s_supported_file_types.push_back(file_type_t(Y("IVF with VP8 video files"),            "ivf"));
  s_supported_file_types.push_back(file_type_t(Y("MP4 audio/video files"),               "mp4 m4v"));
  s_supported_file_types.push_back(file_type_t(Y("MPEG audio files"),                    "mp2 mp3"));
  s_supported_file_types.push_back(file_type_t(Y("MPEG program streams"),                "mpg mpeg m2v mpv evo evob vob"));
  s_supported_file_types.push_back(file_type_t(Y("MPEG transport streams"),              "ts m2ts mts"));
  s_supported_file_types.push_back(file_type_t(Y("MPEG video elementary streams"),       "m1v m2v mpv"));
  s_supported_file_types.push_back(file_type_t(Y("MPLS Blu-ray playlist"),               "mpls"));
  s_supported_file_types.push_back(file_type_t(Y("Matroska audio/video files"),          "mka mks mkv mk3d webm webmv webma"));
  s_supported_file_types.push_back(file_type_t(Y("PGS/SUP subtitles"),                   "sup"));
  s_supported_file_types.push_back(file_type_t(Y("QuickTime audio/video files"),         "mov"));
  s_supported_file_types.push_back(file_type_t(Y("Ogg/OGM audio/video files"),           "ogg ogm ogv"));
  s_supported_file_types.push_back(file_type_t(Y("Opus (in Ogg) audio files"),           "opus ogg"));
  s_supported_file_types.push_back(file_type_t(Y("RealMedia audio/video files"),         "ra ram rm rmvb rv"));
  s_supported_file_types.push_back(file_type_t(Y("SRT text subtitles"),                  "srt"));
  s_supported_file_types.push_back(file_type_t(Y("SSA/ASS text subtitles"),              "ass ssa"));
  s_supported_file_types.push_back(file_type_t(Y("TTA (The lossless True Audio codec)"), "tta"));
  s_supported_file_types.push_back(file_type_t(Y("USF text subtitles"),                  "usf xml"));
  s_supported_file_types.push_back(file_type_t(Y("VC-1 elementary streams"),             "vc1"));
  s_supported_file_types.push_back(file_type_t(Y("VobButtons"),                          "btn"));
  s_supported_file_types.push_back(file_type_t(Y("VobSub subtitles"),                    "idx"));
  s_supported_file_types.push_back(file_type_t(Y("WAVE (uncompressed PCM audio)"),       "wav"));
  s_supported_file_types.push_back(file_type_t(Y("WAVPACK v4 audio"),                    "wv"));
  s_supported_file_types.push_back(file_type_t(Y("WebM audio/video files"),              "webm webmv webma"));
  s_supported_file_types.push_back(file_type_t(Y("WebVTT subtitles"),                    "vtt"));

  return s_supported_file_types;
}

translatable_string_c
file_type_t::get_name(file_type_e type) {
  return file_type_e::aac         == type ? YT("AAC")
       : file_type_e::ac3         == type ? YT("AC-3")
       : file_type_e::asf         == type ? YT("Windows Media (ASF/WMV)")
       : file_type_e::avc_es      == type ? YT("AVC/h.264")
       : file_type_e::avi         == type ? YT("AVI")
       : file_type_e::cdxa        == type ? YT("RIFF CDXA")
       : file_type_e::chapters    == type ? translatable_string_c{} // currently intentionally left blank
       : file_type_e::coreaudio   == type ? YT("CoreAudio")
       : file_type_e::dirac       == type ? YT("Dirac")
       : file_type_e::dts         == type ? YT("DTS")
       : file_type_e::dv          == type ? YT("DV video format")
       : file_type_e::flac        == type ? YT("FLAC")
       : file_type_e::flv         == type ? YT("Flash Video")
       : file_type_e::hdmv_textst == type ? YT("HDMV TextST subtitles")
       : file_type_e::hevc_es     == type ? YT("HEVC/h.265")
       : file_type_e::hdsub       == type ? YT("HD-DVD subtitles")
       : file_type_e::ivf         == type ? YT("IVF (VP8/VP9)")
       : file_type_e::matroska    == type ? YT("Matroska")
       : file_type_e::microdvd    == type ? YT("MicroDVD")
       : file_type_e::mp3         == type ? YT("MP2/MP3")
       : file_type_e::mpeg_es     == type ? YT("MPEG video elementary stream")
       : file_type_e::mpeg_ps     == type ? YT("MPEG program stream")
       : file_type_e::mpeg_ts     == type ? YT("MPEG transport stream")
       : file_type_e::ogm         == type ? YT("Ogg/OGM")
       : file_type_e::pgssup      == type ? YT("PGSSUP")
       : file_type_e::qtmp4       == type ? YT("QuickTime/MP4")
       : file_type_e::real        == type ? YT("RealMedia")
       : file_type_e::srt         == type ? YT("SRT subtitles")
       : file_type_e::ssa         == type ? YT("SSA/ASS subtitles")
       : file_type_e::truehd      == type ? YT("TrueHD")
       : file_type_e::tta         == type ? YT("TTA")
       : file_type_e::usf         == type ? YT("USF subtitles")
       : file_type_e::vc1         == type ? YT("VC-1")
       : file_type_e::vobbtn      == type ? YT("VobBtn")
       : file_type_e::vobsub      == type ? YT("VobSub")
       : file_type_e::wav         == type ? YT("WAV")
       : file_type_e::wavpack4    == type ? YT("WAVPACK")
       : file_type_e::webvtt      == type ? YT("WebVTT subtitles")
       :                                    YT("unknown");
}

}
