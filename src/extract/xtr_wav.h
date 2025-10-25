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
#include "common/wavpack.h"

#include "extract/xtr_base.h"

class xtr_wav_c: public xtr_base_c {
private:
  std::function<void(uint8_t const *, uint8_t *, std::size_t)> m_byte_swapper;
  uint64_t m_channels{}, m_sfreq{}, m_w64_header_size{};
  int m_bps{-1};
  bool m_w64_requested{};

public:
  xtr_wav_c(const std::string &codec_id, int64_t tid, track_spec_t &tspec);

  virtual void create_file(xtr_base_c *master, libmatroska::KaxTrackEntry &track) override;
  virtual void finish_file() override;
  virtual void handle_frame(xtr_frame_t &f) override;

  virtual char const *get_container_name() override;

private:
  virtual void write_w64_header();
  virtual void write_wav_header();
};

class xtr_wavpack4_c: public xtr_base_c {
private:
  uint32_t m_number_of_samples;
  int m_extract_blockadd_level;
  uint8_t m_version[2];
  mm_io_cptr m_corr_out;
  int m_channels;

public:
  xtr_wavpack4_c(const std::string &codec_id, int64_t tid, track_spec_t &tspec);

  virtual void create_file(xtr_base_c *master, libmatroska::KaxTrackEntry &track) override;
  virtual void handle_frame(xtr_frame_t &f) override;
  virtual void finish_file() override;

  virtual const char *get_container_name() override {
    return "WAVPACK";
  };
};
