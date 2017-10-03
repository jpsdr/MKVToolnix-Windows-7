/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definition for the generic packetizer

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include <deque>

#include "common/option_with_source.h"
#include "common/timestamp.h"
#include "common/translation.h"
#include "merge/file_status.h"
#include "merge/packet.h"
#include "merge/timestamp_factory.h"
#include "merge/track_info.h"
#include "merge/webm.h"

namespace libmatroska {
class KaxTrackEntry;
}

using namespace libmatroska;

class generic_reader_c;

enum connection_result_e {
  CAN_CONNECT_YES,
  CAN_CONNECT_NO_FORMAT,
  CAN_CONNECT_NO_PARAMETERS,
  CAN_CONNECT_MAYBE_CODECPRIVATE
};

using packet_cptr_di = std::deque<packet_cptr>::iterator;

class generic_packetizer_c {
protected:
  int m_num_packets;
  std::deque<packet_cptr> m_packet_queue, m_deferred_packets;
  int m_next_packet_wo_assigned_timestamp;

  int64_t m_free_refs, m_next_free_refs, m_enqueued_bytes;
  int64_t m_safety_last_timestamp, m_safety_last_duration;

  KaxTrackEntry *m_track_entry;

  // Header entries. Can be set via set_XXX and will be 'rendered'
  // by set_headers().
  int m_hserialno, m_htrack_type;
  int64_t m_htrack_default_duration;
  bool m_htrack_default_duration_indicates_fields;
  bool m_default_duration_forced;
  bool m_default_track_warning_printed;
  uint64_t m_huid;
  int m_htrack_max_add_block_ids;
  timestamp_c m_seek_pre_roll, m_codec_delay;

  std::string m_hcodec_id;
  memory_cptr m_hcodec_private;

  float m_haudio_sampling_freq, m_haudio_output_sampling_freq;
  int m_haudio_channels, m_haudio_bit_depth;

  int m_hvideo_interlaced_flag, m_hvideo_pixel_width, m_hvideo_pixel_height, m_hvideo_display_width, m_hvideo_display_height;

  compression_method_e m_hcompression;
  compressor_ptr m_compressor;

  timestamp_factory_cptr m_timestamp_factory;
  timestamp_factory_application_e m_timestamp_factory_application_mode;

  int64_t m_last_cue_timestamp;

  bool m_has_been_flushed;

  bool m_prevent_lacing;
  generic_packetizer_c *m_connected_successor;

protected:                      // static
  static int ms_track_number;

public:
  track_info_c m_ti;
  generic_reader_c *m_reader;
  int m_connected_to;
  int64_t m_correction_timestamp_offset;
  int64_t m_append_timestamp_offset, m_max_timestamp_seen;
  bool m_relaxed_timestamp_checking;

public:
  generic_packetizer_c(generic_reader_c *reader, track_info_c &ti);
  virtual ~generic_packetizer_c();

  virtual bool contains_gap();

  virtual file_status_e read(bool force);

  inline void add_packet(packet_t *packet) {
    add_packet(packet_cptr(packet));
  }
  virtual void add_packet(packet_cptr packet);
  virtual void add_packet2(packet_cptr pack);
  virtual void process_deferred_packets();

  virtual packet_cptr get_packet();
  inline bool packet_available() {
    return !m_packet_queue.empty() && m_packet_queue.front()->factory_applied;
  }
  void discard_queued_packets();
  void flush();
  virtual int64_t get_smallest_timestamp() const {
    return m_packet_queue.empty() ? 0x0FFFFFFF : m_packet_queue.front()->timestamp;
  }
  inline int64_t get_queued_bytes() const {
    return m_enqueued_bytes;
  }

  inline void set_free_refs(int64_t free_refs) {
    m_free_refs      = m_next_free_refs;
    m_next_free_refs = free_refs;
  }
  inline int64_t get_free_refs() const {
    return m_free_refs;
  }
  virtual void set_headers();
  virtual void fix_headers();
  inline int process(packet_t *packet) {
    return process(packet_cptr(packet));
  }
  virtual int process(packet_cptr packet) = 0;

  virtual void set_cue_creation(cue_strategy_e create_cue_data) {
    m_ti.m_cues = create_cue_data;
  }
  virtual cue_strategy_e get_cue_creation() const {
    return m_ti.m_cues;
  }
  virtual bool wants_cue_duration() const;
  virtual int64_t get_last_cue_timestamp() const {
    return m_last_cue_timestamp;
  }
  virtual void set_last_cue_timestamp(int64_t timestamp) {
    m_last_cue_timestamp = timestamp;
  }

  virtual KaxTrackEntry *get_track_entry() const {
    return m_track_entry;
  }
  virtual int get_track_num() const {
    return m_hserialno;
  }
  virtual int64_t get_source_track_num() const {
    return m_ti.m_id;
  }

  virtual bool set_uid(uint64_t uid);
  virtual uint64_t get_uid() const {
    return m_huid;
  }
  virtual void set_track_type(int type, timestamp_factory_application_e tfa_mode = TFA_AUTOMATIC);
  virtual int get_track_type() const {
    return m_htrack_type;
  }

  virtual timestamp_c const &get_track_seek_pre_roll() const {
    return m_seek_pre_roll;
  }

  virtual timestamp_c const &get_codec_delay() const {
    return m_codec_delay;
  }

  virtual void set_language(const std::string &language);

  virtual void set_codec_id(const std::string &id);
  virtual void set_codec_private(memory_cptr const &buffer);

  virtual void set_track_default_duration(int64_t default_duration, bool force = false);
  virtual void set_track_max_additionals(int max_add_block_ids);
  virtual int64_t get_track_default_duration() const;
  virtual void set_track_forced_flag(bool forced_track);
  virtual void set_track_enabled_flag(bool enabled_track);
  virtual void set_track_seek_pre_roll(timestamp_c const &seek_pre_roll);
  virtual void set_codec_delay(timestamp_c const &codec_delay);

  virtual void set_audio_sampling_freq(float freq);
  virtual float get_audio_sampling_freq() const {
    return m_haudio_sampling_freq;
  }
  virtual void set_audio_output_sampling_freq(float freq);
  virtual void set_audio_channels(int channels);
  virtual void set_audio_bit_depth(int bit_depth);

  virtual void set_video_interlaced_flag(bool interlaced);
  virtual void set_video_pixel_width(int width);
  virtual void set_video_pixel_height(int height);
  virtual void set_video_pixel_dimensions(int width, int height);
  virtual void set_video_display_width(int width);
  virtual void set_video_display_height(int height);
  virtual void set_video_display_dimensions(int width, int height, option_source_e source);
  virtual void set_video_aspect_ratio(double aspect_ratio, bool is_factor, option_source_e source);
  virtual void set_video_pixel_cropping(int left, int top, int right, int bottom, option_source_e source);
  virtual void set_video_pixel_cropping(const pixel_crop_t &cropping, option_source_e source);
  virtual void set_video_colour_matrix(int matrix_index, option_source_e source);
  virtual void set_video_bits_per_channel(int num_bits, option_source_e source);
  virtual void set_video_chroma_subsample(chroma_subsample_t const &subsample, option_source_e source);
  virtual void set_video_cb_subsample(cb_subsample_t const &subsample, option_source_e source);
  virtual void set_video_chroma_siting(chroma_siting_t const &siting, option_source_e source);
  virtual void set_video_colour_range(int range, option_source_e source);
  virtual void set_video_colour_transfer_character(int transfer_index, option_source_e source);
  virtual void set_video_colour_primaries(int primary_index, option_source_e source);
  virtual void set_video_max_cll(int max_cll, option_source_e source);
  virtual void set_video_max_fall(int max_fall, option_source_e source);
  virtual void set_video_chroma_coordinates(chroma_coordinates_t const &coordinates, option_source_e source);
  virtual void set_video_white_colour_coordinates(white_colour_coordinates_t const &coordinates, option_source_e source);
  virtual void set_video_max_luminance(float max, option_source_e source);
  virtual void set_video_min_luminance(float min, option_source_e source);
  virtual void set_video_projection_type(uint64_t value, option_source_e source);
  virtual void set_video_projection_private(memory_cptr const &value, option_source_e source);
  virtual void set_video_projection_pose_yaw(double value, option_source_e source);
  virtual void set_video_projection_pose_pitch(double value, option_source_e source);
  virtual void set_video_projection_pose_roll(double value, option_source_e source);
  virtual void set_video_field_order(uint64_t order, option_source_e source);
  virtual void set_video_stereo_mode(stereo_mode_c::mode stereo_mode, option_source_e source);
  virtual void set_video_stereo_mode_impl(EbmlMaster &video, stereo_mode_c::mode stereo_mode);

  virtual void set_as_default_track(int type, int priority);

  virtual void set_tag_track_uid();

  virtual void set_track_name(const std::string &name);

  virtual void set_default_compression_method(compression_method_e method) {
    if (COMPRESSION_UNSPECIFIED == m_hcompression)
      m_hcompression = method;
  }

  virtual void force_duration_on_last_packet();

  virtual translatable_string_c get_format_name() const = 0;
  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message) = 0;
  virtual void connect(generic_packetizer_c *src, int64_t append_timestamp_offset = -1);

  virtual void enable_avi_audio_sync(bool enable) {
    m_ti.m_avi_audio_sync_enabled = enable;
  }
  virtual int64_t calculate_avi_audio_sync(int64_t num_bytes, int64_t samples_per_packet, int64_t packet_duration);
  virtual void set_displacement_maybe(int64_t displacement);

  virtual void apply_factory();
  virtual void apply_factory_once(packet_cptr &packet);
  virtual void apply_factory_short_queueing(packet_cptr_di &p_start);
  virtual void apply_factory_full_queueing(packet_cptr_di &p_start);

  virtual bool display_dimensions_or_aspect_ratio_set();

  virtual bool is_compatible_with(output_compatibility_e compatibility);

  int64_t create_track_number();

  virtual void prevent_lacing();
  virtual bool is_lacing_prevented() const;

  virtual generic_packetizer_c *get_connected_successor() const;

  // Callbacks
  virtual void after_packet_timestamped(packet_t &packet);
  virtual void after_packet_rendered(packet_t const &packet);
  virtual void before_file_finished();
  virtual void after_file_created();

protected:
  virtual void flush_impl() {
  };

  virtual void show_experimental_status_version(std::string const &codec_id);

  virtual void compress_packet(packet_t &packet);
  virtual void account_enqueued_bytes(packet_t &packet, int64_t factor);
};

extern std::vector<generic_packetizer_c *> ptzrs_in_header_order;
