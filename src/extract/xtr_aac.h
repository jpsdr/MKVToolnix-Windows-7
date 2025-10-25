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

class xtr_aac_c: public xtr_base_c {
public:
  int m_channels, m_id, m_profile, m_srate_idx;
  memory_cptr m_program_config_element;
  unsigned int m_program_config_element_bit_length{};
  debugging_option_c m_debug{"xtr_aac"};

public:
  xtr_aac_c(const std::string &codec_id, int64_t tid, track_spec_t &tspec);

  virtual void create_file(xtr_base_c *master, libmatroska::KaxTrackEntry &track);
  virtual void handle_frame(xtr_frame_t &f);

  virtual const char *get_container_name() {
    return "raw AAC file with ADTS headers";
  };

protected:
  virtual memory_cptr handle_program_config_element(xtr_frame_t &f);
};
