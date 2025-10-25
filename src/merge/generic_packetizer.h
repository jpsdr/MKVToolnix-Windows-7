/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the generic packetizer

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <deque>

#include "common/option_with_source.h"
#include "common/timestamp.h"
#include "common/translation.h"
#include "merge/block_addition_mapping.h"
#include "merge/file_status.h"
#include "merge/packet.h"
#include "merge/timestamp_factory.h"
#include "merge/track_info.h"
#include "merge/webm.h"

namespace libmatroska {
class KaxTrackEntry;
}

class generic_reader_c;

enum connection_result_e {
  CAN_CONNECT_YES,
  CAN_CONNECT_NO_FORMAT,
  CAN_CONNECT_NO_PARAMETERS,
  CAN_CONNECT_NO_UNSUPPORTED,
  CAN_CONNECT_MAYBE_CODECPRIVATE
};

enum split_result_e {
  CAN_SPLIT_YES,
  CAN_SPLIT_NO_UNSUPPORTED,
};

using packet_cptr_di = std::deque<packet_cptr>::iterator;

class generic_packetizer_c {
public:
  enum display_dimensions_unit_e {
    ddu_pixels       = 0,
    ddu_centimeters  = 1,
    ddu_inches       = 2,
    ddu_aspect_ratio = 3,
    ddu_unknown      = 4,
  };

protected:
  int m_num_packets;
  std::deque<packet_cptr> m_packet_queue, m_deferred_packets;
  int m_next_packet_wo_assigned_timestamp;

  int64_t m_free_refs, m_next_free_refs, m_enqueued_bytes;
  int64_t m_safety_last_timestamp, m_safety_last_duration;

  libmatroska::KaxTrackEntry *m_track_entry;

  // Header entries. Can be set via set_XXX and will be 'rendered'
  // by set_headers().
  int m_hserialno, m_htrack_type;
  int64_t m_htrack_default_duration;
  bool m_htrack_default_duration_indicates_fields;
  bool m_default_duration_forced;
  uint64_t m_huid, m_max_block_add_id{};
  timestamp_c m_seek_pre_roll, m_codec_delay;

  std::string m_hcodec_id, m_hcodec_name;
  memory_cptr m_hcodec_private;

  double m_haudio_sampling_freq, m_haudio_output_sampling_freq;
  int m_haudio_channels, m_haudio_bit_depth;

  int m_hvideo_interlaced_flag, m_hvideo_pixel_width, m_hvideo_pixel_height, m_hvideo_display_width, m_hvideo_display_height, m_hvideo_display_unit;

  std::vector<block_addition_mapping_t> m_block_addition_mappings;

  compression_method_e m_hcompression;
  compressor_ptr m_compressor;

  timestamp_factory_cptr m_timestamp_factory;
  timestamp_factory_application_e m_timestamp_factory_application_mode;

  int64_t m_last_cue_timestamp;

  bool m_has_been_flushed;

  bool m_prevent_lacing;
  generic_packetizer_c *m_connected_successor;

  std::string m_source_id;

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
  generic_packetizer_c(generic_reader_c *reader, track_info_c &ti, track_type type);
  virtual ~generic_packetizer_c();

  virtual bool contains_gap();

  virtual file_status_e read(bool force);

  virtual void add_packet(packet_cptr const &packet);
  virtual void add_packet2(packet_cptr const &pack);
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
  virtual void process(packet_cptr const &packet);

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

  virtual libmatroska::KaxTrackEntry *get_track_entry() const {
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
  void set_track_type(track_type type);
  void set_timestamp_factory_application_mode(timestamp_factory_application_e tfa_mode);
  virtual int get_track_type() const {
    return m_htrack_type;
  }

  virtual timestamp_c const &get_track_seek_pre_roll() const {
    return m_seek_pre_roll;
  }

  virtual timestamp_c const &get_codec_delay() const {
    return m_codec_delay;
  }

  virtual void set_language(mtx::bcp47::language_c const &language);

  virtual void set_codec_id(const std::string &id);
  virtual void set_codec_private(memory_cptr const &buffer);
  virtual void set_codec_name(std::string const &name);

  virtual void set_track_default_duration(int64_t default_duration, bool force = false);
  virtual int64_t get_track_default_duration() const;
  virtual void set_track_default_flag(bool default_track, option_source_e source);
  virtual void set_track_forced_flag(bool forced_track, option_source_e source);
  virtual void set_track_enabled_flag(bool enabled_track, option_source_e source);
  virtual void set_track_seek_pre_roll(timestamp_c const &seek_pre_roll);
  virtual void set_codec_delay(timestamp_c const &codec_delay);
  virtual void set_hearing_impaired_flag(bool hearing_impaired_flag, option_source_e source);
  virtual void set_visual_impaired_flag(bool visual_impaired_flag, option_source_e source);
  virtual void set_text_descriptions_flag(bool text_descriptions_flag, option_source_e source);
  virtual void set_original_flag(bool original_flag, option_source_e source);
  virtual void set_commentary_flag(bool commentary_flag, option_source_e source);

  virtual void set_audio_sampling_freq(double freq);
  virtual double get_audio_sampling_freq() const {
    return m_haudio_sampling_freq;
  }
  virtual void set_audio_output_sampling_freq(double freq);
  virtual void set_audio_channels(int channels);
  virtual void set_audio_bit_depth(int bit_depth);
  virtual void set_audio_emphasis(audio_emphasis_c::mode_e audio_emphasis, option_source_e source);
  virtual void set_audio_emphasis_impl(libebml::EbmlMaster &audio, audio_emphasis_c::mode_e audio_emphasis);

  virtual void set_video_interlaced_flag(bool interlaced);
  virtual void set_video_pixel_dimensions(int width, int height);
  virtual void set_video_display_dimensions(int width, int height, int unit, option_source_e source);
  virtual void set_video_aspect_ratio(double aspect_ratio, bool is_factor, option_source_e source);
  virtual void set_video_pixel_cropping(int left, int top, int right, int bottom, option_source_e source);
  virtual void set_video_pixel_cropping(const pixel_crop_t &cropping, option_source_e source);
  virtual void set_video_color_matrix(uint64_t matrix_index, option_source_e source);
  virtual void set_video_bits_per_channel(uint64_t num_bits, option_source_e source);
  virtual void set_video_chroma_subsample(subsample_or_siting_t const &subsample, option_source_e source);
  virtual void set_video_cb_subsample(subsample_or_siting_t const &subsample, option_source_e source);
  virtual void set_video_chroma_siting(subsample_or_siting_t const &siting, option_source_e source);
  virtual void set_video_color_range(uint64_t range, option_source_e source);
  virtual void set_video_color_transfer_character(uint64_t transfer_index, option_source_e source);
  virtual void set_video_color_primaries(uint64_t primary_index, option_source_e source);
  virtual void set_video_max_cll(uint64_t max_cll, option_source_e source);
  virtual void set_video_max_fall(uint64_t max_fall, option_source_e source);
  virtual void set_video_chroma_coordinates(chroma_coordinates_t const &coordinates, option_source_e source);
  virtual void set_video_white_color_coordinates(white_color_coordinates_t const &coordinates, option_source_e source);
  virtual void set_video_max_luminance(double max, option_source_e source);
  virtual void set_video_min_luminance(double min, option_source_e source);
  virtual void set_video_projection_type(uint64_t value, option_source_e source);
  virtual void set_video_projection_private(memory_cptr const &value, option_source_e source);
  virtual void set_video_projection_pose_yaw(double value, option_source_e source);
  virtual void set_video_projection_pose_pitch(double value, option_source_e source);
  virtual void set_video_projection_pose_roll(double value, option_source_e source);
  virtual void set_video_field_order(uint64_t order, option_source_e source);
  virtual void set_video_stereo_mode(stereo_mode_c::mode stereo_mode, option_source_e source);
  virtual void set_video_stereo_mode_impl(libebml::EbmlMaster &video, stereo_mode_c::mode stereo_mode);
  virtual void set_video_alpha_mode(bool alpha_mode, option_source_e source);
  virtual void set_video_alpha_mode_impl(libebml::EbmlMaster &video, bool alpha_mode);

  virtual void set_block_addition_mappings(std::vector<block_addition_mapping_t> const &mappings);
  virtual void update_max_block_addition_id();
  virtual bool have_block_addition_mapping_type(uint64_t type) const;

  virtual void set_tag_track_uid();

  virtual void set_track_name(const std::string &name);

  virtual void set_default_compression_method(compression_method_e method) {
    if (COMPRESSION_UNSPECIFIED == m_hcompression)
      m_hcompression = method;
  }

  virtual void set_video_color_space(memory_cptr const &value, option_source_e source);

  virtual void force_duration_on_last_packet();

  virtual translatable_string_c get_format_name() const = 0;
  virtual split_result_e can_be_split(std::string &error_message);
  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message) = 0;
  virtual void connect(generic_packetizer_c *src, int64_t append_timestamp_offset = -1);

  virtual void enable_avi_audio_sync(bool enable) {
    m_ti.m_avi_audio_sync_enabled = enable;
  }
  virtual int64_t calculate_avi_audio_sync(int64_t num_bytes, int64_t samples_per_packet, int64_t packet_duration);
  virtual void set_displacement_maybe(int64_t displacement);

  virtual void apply_factory();
  virtual void apply_factory_once(packet_cptr const &packet);
  virtual void apply_factory_short_queueing(packet_cptr_di &p_start);
  virtual void apply_factory_full_queueing(packet_cptr_di &p_start);

  virtual bool display_dimensions_or_aspect_ratio_set();

  virtual bool is_compatible_with(output_compatibility_e compatibility);

  virtual void prevent_lacing();
  virtual bool is_lacing_prevented() const;

  virtual generic_packetizer_c *get_connected_successor() const;

  virtual void set_source_id(std::string const &source_id);
  virtual std::string get_source_id() const;

  // Callbacks
  virtual void after_packet_timestamped(packet_t &packet);
  virtual void after_packet_rendered(packet_t const &packet);
  virtual void before_file_finished();
  virtual void after_file_created();

protected:
  virtual void process_impl(packet_cptr const &packet) = 0;
  virtual void flush_impl() {
  };

  virtual void show_experimental_status_version(std::string const &codec_id);

  virtual void compress_packet(packet_t &packet);
  virtual void account_enqueued_bytes(packet_t &packet, int64_t factor);

  virtual void apply_block_addition_mappings();
};

extern std::vector<generic_packetizer_c *> ptzrs_in_header_order;
