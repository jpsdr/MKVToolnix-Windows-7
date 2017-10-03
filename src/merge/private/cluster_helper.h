/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   The cluster helper groups frames into blocks groups and those
   into clusters, sets the durations, renders the clusters etc.

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/track_statistics.h"

class render_groups_c {
public:
  std::vector<kax_block_blob_cptr> m_groups;
  std::vector<int64_t> m_durations;
  generic_packetizer_c *m_source;
  bool m_more_data, m_duration_mandatory, m_has_discard_padding;

  render_groups_c(generic_packetizer_c *source)
    : m_source(source)
    , m_more_data(false)
    , m_duration_mandatory(false)
    , m_has_discard_padding{}
  {
  }
};
using render_groups_cptr = std::shared_ptr<render_groups_c>;

struct cluster_helper_c::impl_t {
public:
  std::shared_ptr<kax_cluster_c> cluster;
  std::vector<packet_cptr> packets;
  int cluster_content_size{};
  int64_t max_timestamp_and_duration{}, max_video_timestamp_rendered{};
  int64_t previous_cluster_ts{-1}, num_cue_elements{}, header_overhead{-1}, timestamp_offset{};
  int64_t bytes_in_file{}, first_timestamp_in_file{-1}, first_timestamp_in_part{-1}, first_discarded_timestamp{-1}, last_discarded_timestamp_and_duration{}, discarded_duration{}, previous_discarded_duration{};
  timestamp_c min_timestamp_in_file;
  int64_t max_timestamp_in_file{-1}, min_timestamp_in_cluster{-1}, max_timestamp_in_cluster{-1}, frame_field_number{1};
  bool first_video_keyframe_seen{};
  mm_io_c *out{};

  std::vector<split_point_c> split_points;
  std::vector<split_point_c>::iterator current_split_point{split_points.begin()};

  bool discarding{}, splitting_and_processed_fully{};

  chapter_generation_mode_e chapter_generation_mode{chapter_generation_mode_e::none};
  translatable_string_c chapter_generation_name_template{YT("Chapter <NUM:2>")};
  timestamp_c chapter_generation_interval, chapter_generation_last_generated;
  generic_packetizer_c *chapter_generation_reference_track{};
  unsigned int chapter_generation_number{};
  std::string chapter_generation_language;

  std::unordered_map<uint64_t, track_statistics_c> track_statistics;

  debugging_option_c debug_splitting{"cluster_helper|splitting"}, debug_packets{"cluster_helper|cluster_helper_packets"}, debug_duration{"cluster_helper|cluster_helper_duration"},
    debug_rendering{"cluster_helper|cluster_helper_rendering"}, debug_chapter_generation{"cluster_helper|cluster_helper_chapter_generation"};

public:
  ~impl_t();
};
