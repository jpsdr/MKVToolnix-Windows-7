/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definitions for the MPEG PS demultiplexer module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/bit_reader.h"
#include "common/codec.h"
#include "common/debugging.h"
#include "common/dts.h"
#include "common/mm_multi_file_io.h"
#include "common/mpeg1_2.h"
#include "common/strings/formatting.h"
#include "merge/packet_extensions.h"
#include "merge/generic_reader.h"

struct mpeg_ps_id_t {
  int id;
  int sub_id;

  mpeg_ps_id_t(int n_id = 0, int n_sub_id = 0)
    : id(n_id)
    , sub_id(n_sub_id) {
  }

  inline unsigned int idx() const {
    return ((id & 0xff) << 8) | (sub_id & 0xff);
  }
};

class mpeg_ps_packet_c {
public:
  mpeg_ps_id_t m_id;
  int64_t m_pts, m_dts;
  unsigned int m_length, m_full_length;
  bool m_valid;
  memory_cptr m_buffer;

public:
  mpeg_ps_packet_c(mpeg_ps_id_t id = mpeg_ps_id_t{})
    : m_id{id}
    , m_pts{-1}
    , m_dts{-1}
    , m_length{0}
    , m_full_length{0}
    , m_valid{false}
  {
  }

  bool has_pts() const {
    return -1 != m_pts;
  }

  bool has_dts() const {
    return -1 != m_dts;
  }

  int64_t pts() const {
    return has_pts() ? m_pts : std::numeric_limits<int64_t>::max();
  }

  int64_t dts() const {
    return has_dts() ? m_dts : pts();
  }

  operator bool() const {
    return m_valid;
  }

  bool has_been_read() const {
    return m_valid && m_buffer;
  }
};

inline std::ostream &
operator <<(std::ostream &out,
            mpeg_ps_packet_c const &p) {
  out << fmt::format("[ID {0:02x}.{1:02x} PTS {2} DTS {3} length {4} full_length {5} valid? {6} read? {7}]",
                     p.m_id.id, p.m_id.sub_id,
                     p.has_pts() ? mtx::string::format_timestamp(p.pts()) : "none"s,
                     p.has_dts() ? mtx::string::format_timestamp(p.dts()) : "none"s,
                     p.m_length, p.m_full_length, p.m_valid,
                     p.has_been_read() ? "yes, fully" : p.m_buffer ? "only partially" : "no");

  return out;
}

struct mpeg_ps_track_t {
  int ptzr;

  char type;                    // 'v' for video, 'a' for audio, 's' for subs
  mpeg_ps_id_t id;
  int sort_key;
  codec_c codec;

  bool provide_timestamps;
  int64_t timestamp_offset, timestamp_b_frame_offset;

  bool v_interlaced;
  int v_version, v_width, v_height, v_dwidth, v_dheight;
  mtx_mp_rational_t v_field_duration, v_aspect_ratio;

  int a_channels, a_sample_rate, a_bits_per_sample, a_bsid;
  mtx::dts::header_t dts_header;

  uint8_t *buffer;
  unsigned int buffer_usage, buffer_size;

  multiple_timestamps_packet_extension_c *multiple_timestamps_packet_extension;

  unsigned int skip_packet_data_bytes;

  mpeg_ps_track_t():
    ptzr(-1),
    type(0),
    sort_key(0),
    provide_timestamps(false),
    timestamp_offset(std::numeric_limits<int64_t>::max()),
    timestamp_b_frame_offset{0},
    v_interlaced(false),
    v_version(0),
    v_width(0),
    v_height(0),
    v_dwidth(0),
    v_dheight(0),
    a_channels(0),
    a_sample_rate(0),
    a_bits_per_sample(0),
    a_bsid(0),
    buffer(nullptr),
    buffer_usage(0),
    buffer_size(0),
    multiple_timestamps_packet_extension(new multiple_timestamps_packet_extension_c)
    , skip_packet_data_bytes{}
  {
  };

  void use_buffer(size_t size) {
    safefree(buffer);
    buffer       = (uint8_t *)safemalloc(size);
    buffer_size  = size;
    buffer_usage = 0;
  }

  void assert_buffer_size(size_t size) {
    if (!buffer)
      use_buffer(size);
    else if (size > buffer_size) {
      buffer      = (uint8_t *)saferealloc(buffer, size);
      buffer_size = size;
    }
  }

  ~mpeg_ps_track_t() {
    safefree(buffer);
    delete multiple_timestamps_packet_extension;
  }
};
using mpeg_ps_track_ptr = std::shared_ptr<mpeg_ps_track_t>;

inline bool
operator <(mpeg_ps_track_ptr const &a,
           mpeg_ps_track_ptr const &b) {
  return a->sort_key < b->sort_key;
}

class mpeg_ps_reader_c: public generic_reader_c {
private:
  int64_t global_timestamp_offset;

  std::map<int, int> id2idx;
  std::map<int, bool> m_blocked_ids;

  std::map<int, int> es_map;
  int version{};
  bool file_done{};

  std::vector<mpeg_ps_track_ptr> tracks;
  std::map<generic_packetizer_c *, mpeg_ps_track_ptr> m_ptzr_to_track_map;

  uint64_t m_probe_range{};

  debugging_option_c
      m_debug_timestamps{"mpeg_ps|mpeg_ps_timestamps"}
    , m_debug_headers{   "mpeg_ps|mpeg_ps_headers"}
    , m_debug_packets{   "mpeg_ps|mpeg_ps_packets"}
    , m_debug_resync{    "mpeg_ps|mpeg_ps_resync"};

public:
  virtual mtx::file_type_e get_format_type() const {
    return mtx::file_type_e::mpeg_ps;
  }

  virtual void read_headers();
  virtual void identify();
  virtual void create_packetizer(int64_t id);
  virtual void create_packetizers();
  virtual void add_available_track_ids();

  virtual void found_new_stream(mpeg_ps_id_t id);

  virtual bool read_timestamp(mtx::bits::reader_c &bc, int64_t &timestamp);
  virtual bool read_timestamp(int c, int64_t &timestamp);
  virtual mpeg_ps_packet_c parse_packet(mpeg_ps_id_t id, bool read_data = true);
  virtual bool find_next_packet(mpeg_ps_id_t &id, int64_t max_file_pos = -1);
  virtual bool find_next_packet_for_id(mpeg_ps_id_t id, int64_t max_file_pos = -1);

  virtual void parse_program_stream_map();

  virtual bool probe_file() override;

private:
  virtual file_status_e read(generic_packetizer_c *packetizer, bool force = false) override;

  virtual void new_stream_v_avc_or_mpeg_1_2(mpeg_ps_id_t id, uint8_t *buf, unsigned int length, mpeg_ps_track_ptr &track);
  virtual void new_stream_v_mpeg_1_2(mpeg_ps_id_t id, uint8_t *buf, unsigned int length, mpeg_ps_track_ptr &track);
  virtual void new_stream_v_avc(mpeg_ps_id_t id, uint8_t *buf, unsigned int length, mpeg_ps_track_ptr &track);
  virtual void new_stream_v_vc1(mpeg_ps_id_t id, uint8_t *buf, unsigned int length, mpeg_ps_track_ptr &track);
  virtual void new_stream_a_mpeg(mpeg_ps_id_t id, uint8_t *buf, unsigned int length, mpeg_ps_track_ptr &track);
  virtual void new_stream_a_ac3(mpeg_ps_id_t id, uint8_t *buf, unsigned int length, mpeg_ps_track_ptr &track);
  virtual void new_stream_a_dts(mpeg_ps_id_t id, uint8_t *buf, unsigned int length, mpeg_ps_track_ptr &track);
  virtual void new_stream_a_pcm(mpeg_ps_id_t id, uint8_t *buf, unsigned int length, mpeg_ps_track_ptr &track);
  virtual void new_stream_a_truehd(mpeg_ps_id_t id, uint8_t *buf, unsigned int length, mpeg_ps_track_ptr &track);
  virtual bool resync_stream(uint32_t &header);
  virtual file_status_e finish();
  void sort_tracks();
  void calculate_global_timestamp_offset();
};

#if FMT_VERSION >= 90000
template <> struct fmt::formatter<mpeg_ps_id_t>     : ostream_formatter {};
template <> struct fmt::formatter<mpeg_ps_packet_c> : ostream_formatter {};
#endif  // FMT_VERSION >= 90000
