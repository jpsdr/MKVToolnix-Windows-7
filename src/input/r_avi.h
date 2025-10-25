/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definitions for the AVI demultiplexer module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "avilib.h"
#include "common/codec.h"
#include "merge/generic_reader.h"
#include "common/error.h"
#include "input/subtitles.h"

namespace mtx::id {
class info_c;
}

class avc_es_video_packetizer_c;

struct avi_demuxer_t {
  int m_ptzr{-1};
  int m_channels{}, m_bits_per_sample{}, m_samples_per_second{}, m_aid{};
  int64_t m_bytes_processed{};
  codec_c m_codec;
};

struct avi_subs_demuxer_t {
  enum {
    TYPE_UNKNOWN,
    TYPE_SRT,
    TYPE_SSA,
  } m_type;

  int m_ptzr{-1};

  memory_cptr m_subtitles;

  mm_text_io_cptr m_text_io;
  subtitles_cptr m_subs;
  std::optional<std::string> m_encoding;
};

class avi_reader_c: public generic_reader_c {
private:
  enum divx_type_e {
    DIVX_TYPE_NONE,
    DIVX_TYPE_V3,
    DIVX_TYPE_MPEG4
  };

  divx_type_e m_divx_type{DIVX_TYPE_NONE};
  avi_t *m_avi{};
  int m_vptzr{-1};
  std::vector<avi_demuxer_t> m_audio_demuxers;
  std::vector<avi_subs_demuxer_t> m_subtitle_demuxers;
  mtx_mp_rational_t m_default_duration;
  unsigned int m_video_frames_read{}, m_max_video_frames{}, m_dropped_video_frames{};
  unsigned int m_video_width{}, m_video_height{}, m_video_display_width{}, m_video_display_height;
  int m_avc_nal_size_size{-1};

  uint64_t m_bytes_to_process{}, m_bytes_processed{};
  bool m_video_track_ok{};

  debugging_option_c m_debug_aspect_ratio{"avi|avi_aspect_ratio"}, m_debug{"avi|avi_reader"};

public:
  virtual ~avi_reader_c();

  virtual mtx::file_type_e get_format_type() const {
    return mtx::file_type_e::avi;
  }

  virtual void read_headers();
  virtual int64_t get_progress() override;
  virtual int64_t get_maximum_progress() override;
  virtual void identify();
  virtual void create_packetizers();
  virtual void create_packetizer(int64_t tid);
  virtual void add_available_track_ids();

  virtual bool probe_file() override;

protected:
  virtual void add_audio_demuxer(int aid);
  virtual file_status_e read(generic_packetizer_c *packetizer, bool force = false) override;
  virtual file_status_e read_video();
  virtual file_status_e read_audio(avi_demuxer_t &demuxer);
  virtual file_status_e read_subtitles(avi_subs_demuxer_t &demuxer);

  virtual void handle_video_aspect_ratio();

  virtual generic_packetizer_c *create_aac_packetizer(int aid, avi_demuxer_t &demuxer);
  virtual generic_packetizer_c *create_dts_packetizer(int aid);
  virtual generic_packetizer_c *create_vorbis_packetizer(int aid);
  virtual void create_subs_packetizer(int idx);
  virtual void create_srt_packetizer(int idx);
  virtual void create_ssa_packetizer(int idx);
  virtual void create_standard_video_packetizer();
  virtual void create_mpeg1_2_packetizer();
  virtual void create_mpeg4_p2_packetizer();
  virtual void create_mpeg4_p10_packetizer();
  virtual void create_vp8_packetizer();
  virtual void create_video_packetizer();

  virtual void set_avc_nal_size_size(avc_es_video_packetizer_c *packetizer);

  void extended_identify_mpeg4_l2(mtx::id::info_c &info);

  void parse_subtitle_chunks();
  void verify_video_track();

  virtual void identify_video();
  virtual void identify_audio();
  virtual void identify_subtitles();
  virtual void identify_attachments();

  virtual void debug_dump_video_index();
};
