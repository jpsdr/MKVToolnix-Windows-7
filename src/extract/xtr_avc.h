/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   extracts tracks from Matroska files into other files

   Written by Matt Rice <topquark@sluggy.net>.
*/

#pragma once

#include "common/common_pch.h"

#include "extract/xtr_base.h"

using nal_unit_list_t = std::vector< std::pair<memory_cptr, unsigned char> >;

class xtr_avc_c: public xtr_base_c {
protected:
  int m_nal_size_size;

  static binary const ms_start_code[4];

public:
  xtr_avc_c(const std::string &codec_id, int64_t tid, track_spec_t &tspec);

  virtual void create_file(xtr_base_c *master, KaxTrackEntry &track) override;
  virtual void handle_frame(xtr_frame_t &f) override;
  virtual bool write_nal(const binary *data, size_t &pos, size_t data_size, size_t nal_size_size);

  virtual const char *get_container_name() override {
    return "AVC/h.264 elementary stream";
  };

  virtual nal_unit_list_t find_nal_units(binary *buf, std::size_t frame_size) const;
};
