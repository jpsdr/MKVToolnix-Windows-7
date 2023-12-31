/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   extracts tracks from Matroska files into other files

*/

#pragma once

#include "common/common_pch.h"

#include "common/hevc/es_parser.h"
#include "extract/xtr_base.h"
#include "extract/xtr_avc.h"

class xtr_hevc_c: public xtr_avc_c {
protected:
  memory_cptr m_decoded_codec_private;
  debugging_option_c m_debug{"xtr_hevc|hevc"};
  bool m_first_nalu{true}, m_normalize_parameter_sets{true};
  mtx::hevc::es_parser_c m_parser;

public:
  xtr_hevc_c(const std::string &codec_id, int64_t tid, track_spec_t &tspec);

  virtual const char *get_container_name() override {
    return "HEVC/H.265 elementary stream";
  };

  virtual void create_file(xtr_base_c *master, libmatroska::KaxTrackEntry &track) override;
  virtual void handle_frame(xtr_frame_t &f) override;
  virtual bool write_nal(uint8_t *data, size_t &pos, size_t data_size, size_t write_nal_size_size) override;

  virtual void finish_track() override;

protected:
  virtual void flush_frames();
  virtual void unwrap_write_hevcc(bool skip_prefix_sei);
};
