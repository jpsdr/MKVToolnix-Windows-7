/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   extracts tracks from Matroska files into other files

*/

#pragma once

#include "common/common_pch.h"

#include "extract/xtr_base.h"
#include "extract/xtr_avc.h"

class xtr_hevc_c: public xtr_avc_c {
protected:
  bool m_first_nalu{true}, m_check_for_sei_in_first_frame{true}, m_drop_vps_sps_pps_in_frame{true};
  memory_cptr m_decoded_codec_private;

public:
  xtr_hevc_c(const std::string &codec_id, int64_t tid, track_spec_t &tspec) : xtr_avc_c(codec_id, tid, tspec) { }

  virtual const char *get_container_name() override {
    return "HEVC/h.265 elementary stream";
  };

  virtual void create_file(xtr_base_c *master, KaxTrackEntry &track) override;
  virtual void handle_frame(xtr_frame_t &f) override;
  virtual bool write_nal(const binary *data, size_t &pos, size_t data_size, size_t write_nal_size_size) override;

protected:
  virtual void unwrap_write_hevcc(bool skip_sei);
  virtual void check_for_sei_in_first_frame(nal_unit_list_t const &nal_units);
};
