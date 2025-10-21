/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   extracts tracks from Matroska files into other files

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "extract/xtr_base.h"

class xtr_tta_c: public xtr_base_c {
public:
  std::vector<int64_t> m_frame_sizes;
  int64_t m_previous_duration;
  int m_bps, m_channels, m_sfreq;
  std::string m_temp_file_name;

  static const double ms_tta_frame_time;

public:
  xtr_tta_c(const std::string &codec_id, int64_t tid, track_spec_t &tspec);

  virtual void create_file(xtr_base_c *master, libmatroska::KaxTrackEntry &track);
  virtual void handle_frame(xtr_frame_t &f);
  virtual void finish_file();

  virtual const char *get_container_name() {
    return "TTA (TrueAudio)";
  };
};
