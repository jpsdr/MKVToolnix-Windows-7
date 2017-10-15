/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   File type enum

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/translation.h"

namespace mtx {

// File types

// !!!!!!!!!!!!! ATTENTION !!!!!!!!!!!!!

// These values are stored in .mtxcfg files as types.  Therefore their
// numbers must not change. New file types must always only be added
// at the end now, even if it disturbs the current alphabetical order.

// !!!!!!!!!!!!! ATTENTION !!!!!!!!!!!!!
enum class file_type_e {
  is_unknown = 0,
  aac,
  ac3,
  asf,
  avc_es,
  avi,
  cdxa,
  chapters,
  coreaudio,
  dirac,
  dts,
  dv,
  flac,
  flv,
  hevc_es,
  hdsub,
  ivf,
  matroska,
  microdvd,
  mp3,
  mpeg_es,
  mpeg_ps,
  mpeg_ts,
  ogm,
  pgssup,
  qtmp4,
  real,
  srt,
  ssa,
  truehd,
  tta,
  usf,
  vc1,
  vobbtn,
  vobsub,
  wav,
  wavpack4,
  webvtt,
  hdmv_textst,
  max = hdmv_textst,
};

struct file_type_t {
  std::string title, extensions;

  file_type_t(const std::string &p_title, const std::string &p_extensions)
    : title(p_title)
    , extensions(p_extensions)
  {
  }

  static std::vector<file_type_t> &get_supported();
  static translatable_string_c get_name(file_type_e type);
};

}
