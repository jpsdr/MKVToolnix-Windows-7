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

#include "common/timestamp.h"
#include "extract/xtr_base.h"

class xtr_vobsub_c: public xtr_base_c {
public:
  std::vector<int64_t> m_positions, m_timestamps;
  std::vector<xtr_vobsub_c *> m_slaves;
  memory_cptr m_private_data;
  boost::filesystem::path m_idx_file_name, m_sub_file_name;
  mtx::bcp47::language_c m_language;
  int m_stream_id;

public:
  xtr_vobsub_c(const std::string &codec_id, int64_t tid, track_spec_t &tspec);

  virtual void create_file(xtr_base_c *master, libmatroska::KaxTrackEntry &track);
  virtual void handle_frame(xtr_frame_t &f);
  virtual void finish_file();
  virtual void write_idx(mm_io_c &idx, int index);

  virtual boost::filesystem::path get_file_name() const;
  virtual const char *get_container_name() {
    return "VobSubs";
  };

protected:
  void fix_spu_duration(memory_c &buffer, timestamp_c const &duration) const;
};
