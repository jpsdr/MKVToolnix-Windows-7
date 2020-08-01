/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Extraction of Blu-Ray graphics subtitles.

   Written by Moritz Bunkus and Mike Chen.
*/

#pragma once

#include "common/common_pch.h"

#include "extract/xtr_base.h"

class xtr_hdmv_pgs_c: public xtr_base_c {
  debugging_option_c m_debug{"hdmv_pgs|xtr_hdmv_pgs"};

public:
  xtr_hdmv_pgs_c(const std::string &codec_id, int64_t tid, track_spec_t &tspec);

  virtual void handle_frame(xtr_frame_t &f);

  virtual const char *get_container_name() {
    return "SUP";
  }
};
