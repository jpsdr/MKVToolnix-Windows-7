/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   extracts tracks from Matroska files into other files

   Written by Matt Rice <topquark@sluggy.net>.
*/

#pragma once

#include "common/common_pch.h"

#include "extract/xtr_base.h"

class xtr_mpeg1_2_video_c: public xtr_base_c {
public:
  memory_cptr m_seq_hdr;

public:
  xtr_mpeg1_2_video_c(const std::string &codec_id, int64_t tid, track_spec_t &tspec);

  virtual void create_file(xtr_base_c *master, libmatroska::KaxTrackEntry &track);
  virtual void handle_frame(xtr_frame_t &f);
  virtual void handle_codec_state(memory_cptr &codec_state);

  virtual const char *get_container_name() {
    return "MPEG-1/-2 elementary stream";
  };
};
