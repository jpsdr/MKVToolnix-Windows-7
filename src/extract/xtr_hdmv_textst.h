/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Extraction of Blu-Ray text subtitles.

   Written by Moritz Bunkus.
*/

#pragma once

#include "common/common_pch.h"

#include "common/timestamp.h"
#include "extract/xtr_base.h"

class xtr_hdmv_textst_c: public xtr_base_c {
protected:
  unsigned int m_num_presentation_segments;
  int64_t m_num_presentation_segment_position;

public:
  xtr_hdmv_textst_c(const std::string &codec_id, int64_t tid, track_spec_t &tspec);

  virtual void create_file(xtr_base_c *master, libmatroska::KaxTrackEntry &track);
  virtual void finish_file();
  virtual void handle_frame(xtr_frame_t &f);

  virtual const char *get_container_name() {
    return Y("HDMV TextST subtitles");
  }

protected:
  void put_timestamp(uint8_t *buf, timestamp_c const &timestamp);
};
