/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   cues storage & helper class

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <matroska/KaxCues.h>
#include <matroska/KaxCuesData.h>
#include <matroska/KaxSeekHead.h>

using id_timestamp_t = std::pair<uint64_t, uint64_t>;

struct cue_point_t {
  uint64_t timestamp, duration, cluster_position;
  uint32_t track_num, relative_position;
};

class cues_c;
using cues_cptr = std::shared_ptr<cues_c>;

class cues_c {
protected:
  std::vector<cue_point_t> m_points;
  std::multimap<id_timestamp_t, uint64_t> m_id_timestamp_duration_multimap;
  std::map<id_timestamp_t, uint64_t> m_codec_state_position_map;

  size_t m_num_cue_points_postprocessed;
  bool m_no_cue_duration, m_no_cue_relative_position;
  debugging_option_c m_debug_cue_duration, m_debug_cue_relative_position;

protected:
  static cues_cptr s_cues;

public:
  cues_c();

  void add(libmatroska::KaxCues &cues);
  void add(libmatroska::KaxCuePoint &point);
  void write(mm_io_c &out, libmatroska::KaxSeekHead &seek_head);
  void postprocess_cues(libmatroska::KaxCues &cues, libmatroska::KaxCluster &cluster);
  void set_duration_for_id_timestamp(uint64_t id, uint64_t timestamp, uint64_t duration);
  void adjust_positions(uint64_t old_position, uint64_t delta);

public:
  static cues_c &get();

protected:
  void sort();
  std::multimap<id_timestamp_t, uint64_t> calculate_block_positions(libmatroska::KaxCluster &cluster) const;
  uint64_t calculate_total_size() const;
  uint64_t calculate_point_size(cue_point_t const &point) const;
  uint64_t calculate_bytes_for_uint(uint64_t value) const;
};
