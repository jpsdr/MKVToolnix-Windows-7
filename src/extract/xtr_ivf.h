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

#include "common/ivf.h"
#include "extract/xtr_base.h"

class xtr_ivf_c: public xtr_base_c {
public:
  uint64_t m_frame_rate_num{}, m_frame_rate_den{};
  uint32_t m_frame_count{};
  ivf::file_header_t m_file_header;
  bool m_is_av1{};
  debugging_option_c m_debug{"ivf|xtr_ivf"};

public:
  xtr_ivf_c(const std::string &codec_id, int64_t tid, track_spec_t &tspec);

  virtual void create_file(xtr_base_c *master, libmatroska::KaxTrackEntry &track);
  virtual void handle_frame(xtr_frame_t &f);
  virtual void finish_file();

  virtual const char *get_container_name() {
    return "IVF";
  };

protected:
  virtual void av1_prepend_temporal_delimiter_obu_if_needed(memory_c &frame);
};
