/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   The cluster helper groups frames into blocks groups and those
   into clusters, sets the durations, renders the clusters etc.

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef MTX_MERGE_PRIVATE_CLUSTER_HELPER_H
#define MTX_MERGE_PRIVATE_CLUSTER_HELPER_H

#include "merge/track_statistics.h"

class render_groups_c {
public:
  std::vector<kax_block_blob_cptr> m_groups;
  std::vector<int64_t> m_durations;
  generic_packetizer_c *m_source;
  bool m_more_data, m_duration_mandatory;

  render_groups_c(generic_packetizer_c *source)
    : m_source(source)
    , m_more_data(false)
    , m_duration_mandatory(false)
  {
  }
};
typedef std::shared_ptr<render_groups_c> render_groups_cptr;

struct cluster_helper_c::impl_t {
public:
  kax_cluster_c *cluster;
  std::vector<packet_cptr> packets;
  int cluster_content_size;
  int64_t max_timecode_and_duration, max_video_timecode_rendered;
  int64_t previous_cluster_tc, num_cue_elements, header_overhead;
  int64_t timecode_offset;
  int64_t bytes_in_file, first_timecode_in_file, first_timecode_in_part, first_discarded_timecode, last_discarded_timecode_and_duration, discarded_duration, previous_discarded_duration;
  timecode_c min_timecode_in_file;
  int64_t max_timecode_in_file, min_timecode_in_cluster, max_timecode_in_cluster, frame_field_number;
  bool first_video_keyframe_seen;
  mm_io_c *out;

  std::vector<split_point_c> split_points;
  std::vector<split_point_c>::iterator current_split_point;

  bool discarding, splitting_and_processed_fully;

  debugging_option_c debug_splitting, debug_packets, debug_duration, debug_rendering;

  std::unordered_map<uint64_t, track_statistics_c> track_statistics;

public:
  impl_t();
  ~impl_t();
};

#endif // MTX_MERGE_PRIVATE_CLUSTER_HELPER_H
