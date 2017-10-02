/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   extracts tracks from Matroska files into other files

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "extract/xtr_base.h"

class xtr_wav_c: public xtr_base_c {
private:
  wave_header m_wh;
  std::function<void(unsigned char const *, unsigned char *, std::size_t)> m_byte_swapper;

public:
  xtr_wav_c(const std::string &codec_id, int64_t tid, track_spec_t &tspec);

  virtual void create_file(xtr_base_c *master, KaxTrackEntry &track) override;
  virtual void finish_file() override;
  virtual void handle_frame(xtr_frame_t &f) override;

  virtual const char *get_container_name() override {
    return "WAV";
  };
};

class xtr_wavpack4_c: public xtr_base_c {
private:
  uint32_t m_number_of_samples;
  int m_extract_blockadd_level;
  binary m_version[2];
  mm_io_cptr m_corr_out;
  int m_channels;

public:
  xtr_wavpack4_c(const std::string &codec_id, int64_t tid, track_spec_t &tspec);

  virtual void create_file(xtr_base_c *master, KaxTrackEntry &track) override;
  virtual void handle_frame(xtr_frame_t &f) override;
  virtual void finish_file() override;

  virtual const char *get_container_name() override {
    return "WAVPACK";
  };
};
