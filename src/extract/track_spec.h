/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   extracts tracks and other items from Matroska files into other files

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

struct track_spec_t {
  enum target_mode_e {
    tm_normal,
    tm_raw,
    tm_full_raw,
    tm_timestamps,
  };

  int64_t tid, tuid;
  std::string out_name;

  std::string sub_charset;
  bool extract_cuesheet;

  target_mode_e target_mode;
  int extract_blockadd_level;

  bool done;

  track_spec_t();

  void dump(std::string const &prefix) const;
};
