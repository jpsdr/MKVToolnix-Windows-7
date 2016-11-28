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
  return FILE_TYPE_AAC         == type ? YT("AAC")
       : FILE_TYPE_AC3         == type ? YT("AC-3")
       : FILE_TYPE_ASF         == type ? YT("Windows Media (ASF/WMV)")
       : FILE_TYPE_AVC_ES      == type ? YT("AVC/h.264")
       : FILE_TYPE_AVI         == type ? YT("AVI")
       : FILE_TYPE_CDXA        == type ? YT("RIFF CDXA")
       : FILE_TYPE_CHAPTERS    == type ? translatable_string_c{} // currently intentionally left blank
       : FILE_TYPE_COREAUDIO   == type ? YT("CoreAudio")
       : FILE_TYPE_DIRAC       == type ? YT("Dirac")
       : FILE_TYPE_DTS         == type ? YT("DTS")
       : FILE_TYPE_DV          == type ? YT("DV video format")
       : FILE_TYPE_FLAC        == type ? YT("FLAC")
       : FILE_TYPE_FLV         == type ? YT("Flash Video")
       : FILE_TYPE_HDMV_TEXTST == type ? YT("HDMV TextST subtitles")
       : FILE_TYPE_HEVC_ES     == type ? YT("HEVC/h.265")
       : FILE_TYPE_HDSUB       == type ? YT("HD-DVD subtitles")
       : FILE_TYPE_IVF         == type ? YT("IVF (VP8/VP9)")
       : FILE_TYPE_MATROSKA    == type ? YT("Matroska")
       : FILE_TYPE_MICRODVD    == type ? YT("MicroDVD")
       : FILE_TYPE_MP3         == type ? YT("MP2/MP3")
       : FILE_TYPE_MPEG_ES     == type ? YT("MPEG video elementary stream")
       : FILE_TYPE_MPEG_PS     == type ? YT("MPEG program stream")
       : FILE_TYPE_MPEG_TS     == type ? YT("MPEG transport stream")
       : FILE_TYPE_OGM         == type ? YT("Ogg/OGM")
       : FILE_TYPE_PGSSUP      == type ? YT("PGSSUP")
       : FILE_TYPE_QTMP4       == type ? YT("QuickTime/MP4")
       : FILE_TYPE_REAL        == type ? YT("RealMedia")
       : FILE_TYPE_SRT         == type ? YT("SRT subtitles")
       : FILE_TYPE_SSA         == type ? YT("SSA/ASS subtitles")
       : FILE_TYPE_TRUEHD      == type ? YT("TrueHD")
       : FILE_TYPE_TTA         == type ? YT("TTA")
       : FILE_TYPE_USF         == type ? YT("USF subtitles")
       : FILE_TYPE_VC1         == type ? YT("VC-1")
       : FILE_TYPE_VOBBTN      == type ? YT("VobBtn")
       : FILE_TYPE_VOBSUB      == type ? YT("VobSub")
       : FILE_TYPE_WAV         == type ? YT("WAV")
       : FILE_TYPE_WAVPACK4    == type ? YT("WAVPACK")
       : FILE_TYPE_WEBVTT      == type ? YT("WebVTT subtitles")
       :                                 YT("unknown");
}
