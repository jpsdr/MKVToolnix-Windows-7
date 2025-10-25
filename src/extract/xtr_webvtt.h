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

#include "common/xml/xml.h"
#include "extract/xtr_base.h"

class xtr_webvtt_c: public xtr_base_c {
protected:
  unsigned int m_num_entries{};

public:
  xtr_webvtt_c(const std::string &codec_id, int64_t tid, track_spec_t &tspec);

  virtual void create_file(xtr_base_c *master, libmatroska::KaxTrackEntry &track);
  virtual void handle_frame(xtr_frame_t &f);

  virtual const char *get_container_name() {
    return Y("WebVTT subtitles");
  };
};
