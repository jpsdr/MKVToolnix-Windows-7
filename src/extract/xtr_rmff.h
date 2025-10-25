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

#include "librmff/librmff.h"

#include "extract/xtr_base.h"

class xtr_rmff_c: public xtr_base_c {
public:
  rmff_file_t *m_file;
  rmff_track_t *m_rmtrack;

public:
  xtr_rmff_c(const std::string &codec_id, int64_t tid, track_spec_t &tspec);

  virtual void create_file(xtr_base_c *master, libmatroska::KaxTrackEntry &track);
  virtual void handle_frame(xtr_frame_t &f);
  virtual void finish_file();
  virtual void headers_done();

  virtual const char *get_container_name() {
    return "RMFF (RealMedia File Format)";
  };
};
