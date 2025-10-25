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

#include <matroska/KaxBlock.h>
#include <matroska/KaxTracks.h>

#include "common/bcp47.h"
#include "common/content_decoder.h"
#include "common/path.h"
#include "common/timestamp.h"
#include "extract/mkvextract.h"

struct xtr_frame_t {
  memory_cptr &frame;
  libmatroska::KaxBlockAdditions *additions;
  int64_t timestamp, duration, bref, fref;
  bool keyframe, discardable;
  timestamp_c discard_duration;
};

class xtr_base_c {
public:
  std::string m_codec_id, m_file_name, m_container_name;
  xtr_base_c *m_master;
  mm_io_cptr m_out;
  int64_t m_tid, m_track_num;
  int64_t m_default_duration;

  int64_t m_bytes_written;

  content_decoder_c m_content_decoder;
  bool m_content_decoder_initialized;

  bool m_debug;

public:
  xtr_base_c(const std::string &codec_id, int64_t tid, track_spec_t &tspec, const char *container_name = nullptr);
  virtual ~xtr_base_c();

  void decode_and_handle_frame(xtr_frame_t &f);

  virtual void create_file(xtr_base_c *_master, libmatroska::KaxTrackEntry &track);
  virtual void handle_frame(xtr_frame_t &f);
  virtual void handle_codec_state(memory_cptr &/* codec_state */) {
  };
  virtual void finish_track();
  virtual void finish_file();

  virtual void headers_done();

  virtual boost::filesystem::path get_file_name() const {
    return mtx::fs::to_path(m_file_name);
  }
  virtual const char *get_container_name() {
    return m_container_name.c_str();
  };

  virtual mtx::bcp47::language_c get_track_language(libmatroska::KaxTrackEntry &track);

  virtual void init_content_decoder(libmatroska::KaxTrackEntry &track);
  virtual memory_cptr decode_codec_private(libmatroska::KaxCodecPrivate *priv);

  static std::shared_ptr<xtr_base_c> create_extractor(const std::string &new_codec_id, int64_t new_tid, track_spec_t &tspec);
};

class xtr_fullraw_c : public xtr_base_c {
public:
  xtr_fullraw_c(const std::string &codec_id, int64_t tid, track_spec_t &tspec):
    xtr_base_c(codec_id, tid, tspec) {}
  virtual void create_file(xtr_base_c *master, libmatroska::KaxTrackEntry &track);
  virtual void handle_codec_state(memory_cptr &codec_state);
};
