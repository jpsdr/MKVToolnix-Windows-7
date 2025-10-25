/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   cues storage & helper class

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/debugging.h"
#include "common/doc_type_version_handler.h"
#include "common/ebml.h"
#include "common/hacks.h"
#include "merge/cues.h"
#include "merge/generic_packetizer.h"
#include "merge/libmatroska_extensions.h"
#include "merge/output_control.h"

cues_cptr cues_c::s_cues;

cues_c::cues_c()
  : m_num_cue_points_postprocessed{}
  , m_no_cue_duration{mtx::hacks::is_engaged(mtx::hacks::NO_CUE_DURATION)}
  , m_no_cue_relative_position{mtx::hacks::is_engaged(mtx::hacks::NO_CUE_RELATIVE_POSITION)}
  , m_debug_cue_duration{         "cues|cues_cue_duration"}
  , m_debug_cue_relative_position{"cues|cues_cue_relative_position"}
{
}

void
cues_c::set_duration_for_id_timestamp(uint64_t id,
                                     uint64_t timestamp,
                                     uint64_t duration) {
  if (!m_no_cue_duration)
    m_id_timestamp_duration_multimap.insert({ id_timestamp_t{id, timestamp}, duration });
}

void
cues_c::add(libmatroska::KaxCues &cues) {
  for (auto child : cues) {
    auto point = dynamic_cast<libmatroska::KaxCuePoint *>(child);
    if (point)
      add(*point);
  }
}

void
cues_c::add(libmatroska::KaxCuePoint &point) {
  uint64_t timestamp = find_child_value<libmatroska::KaxCueTime>(point) * g_timestamp_scale;

  for (auto point_child : point) {
    auto positions = dynamic_cast<libmatroska::KaxCueTrackPositions *>(point_child);
    if (!positions)
      continue;

    uint64_t track_num = find_child_value<libmatroska::KaxCueTrack>(*positions);
    assert(track_num <= static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()));

    m_points.push_back({ timestamp, 0, find_child_value<libmatroska::KaxCueClusterPosition>(*positions), static_cast<uint32_t>(track_num), 0 });

    uint64_t codec_state_position = find_child_value<libmatroska::KaxCueCodecState>(*positions);
    if (codec_state_position)
      m_codec_state_position_map[ id_timestamp_t{ track_num, timestamp } ] = codec_state_position;
  }
}

void
cues_c::write(mm_io_c &out,
              libmatroska::KaxSeekHead &seek_head) {
  if (!m_points.size() || !g_cue_writing_requested)
    return;

  // auto start = mtx::sys::get_current_time_millis();
  sort();
  // auto end_sort = mtx::sys::get_current_time_millis();

  // Need to write the (empty) cues element so that its position will
  // be set for indexing in g_kax_sh_main. Necessary because there's
  // no API function to force the position to a certain value; nor is
  // there a different API function in libmatroska::KaxSeekHead for adding anything
  // by ID and position manually.
  out.save_pos();
  kax_cues_position_dummy_c cues_dummy;
  cues_dummy.Render(out);
  out.restore_pos();

  // Write meta seek information if it is not disabled.
  seek_head.IndexThis(cues_dummy, *g_kax_segment);

  // Forcefully write the correct head and copy its content from the
  // temporary storage location.
  auto total_size = calculate_total_size();
  write_ebml_element_head(out, EBML_ID(libmatroska::KaxCues), total_size);

  for (auto &point : m_points) {
    libmatroska::KaxCuePoint kc_point;

    get_child<libmatroska::KaxCueTime>(kc_point).SetValue(point.timestamp / g_timestamp_scale);

    auto &positions = get_child<libmatroska::KaxCueTrackPositions>(kc_point);
    get_child<libmatroska::KaxCueTrack>(positions).SetValue(point.track_num);
    get_child<libmatroska::KaxCueClusterPosition>(positions).SetValue(point.cluster_position);

    auto codec_state_position = m_codec_state_position_map.find({ point.track_num, point.timestamp });
    if (codec_state_position != m_codec_state_position_map.end())
      get_child<libmatroska::KaxCueCodecState>(positions).SetValue(codec_state_position->second);

    if (point.relative_position)
      get_child<libmatroska::KaxCueRelativePosition>(positions).SetValue(point.relative_position);

    if (point.duration)
      get_child<libmatroska::KaxCueDuration>(positions).SetValue(round_timestamp_scale(point.duration) / g_timestamp_scale);

    g_doc_type_version_handler->render(kc_point, out);
  }

  m_points.clear();
  m_codec_state_position_map.clear();
  m_num_cue_points_postprocessed = 0;

  // auto end_all = mtx::sys::get_current_time_millis();
  // mxinfo(fmt::format("dur sort {0} write {1} total {2}\n", end_sort - start, end_all - end_sort, end_all - start));
}

void
cues_c::sort() {
  std::sort(m_points.begin(), m_points.end(), [](cue_point_t const &a, cue_point_t const &b) -> bool {
      if (a.timestamp < b.timestamp)
        return true;
      if (a.timestamp > b.timestamp)
        return false;

      if (a.track_num < b.track_num)
        return true;

      return false;
    });
}

std::multimap<id_timestamp_t, uint64_t>
cues_c::calculate_block_positions(libmatroska::KaxCluster &cluster)
  const {

  std::multimap<id_timestamp_t, uint64_t> positions;

  for (auto child : cluster) {
    auto simple_block = dynamic_cast<libmatroska::KaxSimpleBlock *>(child);
    if (simple_block) {
      simple_block->SetParent(cluster);
      positions.insert({ id_timestamp_t{ simple_block->TrackNum(), get_global_timestamp(*simple_block)}, simple_block->GetElementPosition() });
      continue;
    }

    auto block_group = dynamic_cast<libmatroska::KaxBlockGroup *>(child);
    if (!block_group)
      continue;

    auto block = find_child<libmatroska::KaxBlock>(block_group);
    if (!block)
      continue;

    block->SetParent(cluster);
    positions.insert({ id_timestamp_t{ block->TrackNum(), get_global_timestamp(*block)}, block_group->GetElementPosition() });
  }

  return positions;
}

void
cues_c::postprocess_cues(libmatroska::KaxCues &cues,
                         libmatroska::KaxCluster &cluster) {
  add(cues);

  if (m_no_cue_duration && m_no_cue_relative_position)
    return;

  auto cluster_data_start_pos = cluster.GetDataStart();
  auto block_positions        = calculate_block_positions(cluster);
  std::map<id_timestamp_t, size_t> nblocks_processed; //# blocks processed so far with given track #/timestamp

  for (auto point = m_points.begin() + m_num_cue_points_postprocessed, end = m_points.end(); point != end; ++point) {
    nblocks_processed[id_timestamp_t{ point->track_num, point->timestamp }]++;

    // Set CueRelativePosition for all cues.
    if (!m_no_cue_relative_position) {
      auto pair          = block_positions.equal_range({ point->track_num, point->timestamp });
      auto position_itr  = pair.first;
      auto pos_end       = pair.second;
      auto num_processed = nblocks_processed[id_timestamp_t{ point->track_num, point->timestamp }];

      for (auto i = 0u; ((i + 1) < num_processed) && (position_itr != pos_end); ++i)
        position_itr++;

      auto relative_position = block_positions.end() != position_itr ? std::max(position_itr->second, cluster_data_start_pos) - cluster_data_start_pos : 0ull;

      assert(relative_position <= static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()));

      point->relative_position = relative_position;

      mxdebug_if(m_debug_cue_relative_position,
                 fmt::format("cue_relative_position: looking for <{0}:{1}>: cluster_data_start_pos {2} position {3}\n",
                             point->track_num, point->timestamp, cluster_data_start_pos, relative_position));
    }

    // Set CueDuration if the packetizer wants them.
    if (m_no_cue_duration)
      continue;

    auto pair          = m_id_timestamp_duration_multimap.equal_range({ point->track_num, point->timestamp });
    auto duration_itr  = pair.first;
    auto dur_end       = pair.second;
    auto num_processed = nblocks_processed[id_timestamp_t{ point->track_num, point->timestamp }];

    for (auto i = 0u; ((i + 1) < num_processed) && (duration_itr != dur_end); ++i)
      duration_itr++;

    auto ptzr         = g_packetizers_by_track_num[point->track_num];

    if (!ptzr || !ptzr->wants_cue_duration())
      continue;

    if (m_id_timestamp_duration_multimap.end() != duration_itr)
      point->duration = duration_itr->second;

    mxdebug_if(m_debug_cue_duration,
               fmt::format("cue_duration: looking for <{0}:{1}>: {2}\n",
                           point->track_num, point->timestamp, duration_itr == m_id_timestamp_duration_multimap.end() ? static_cast<int64_t>(-1) : duration_itr->second));
  }

  m_num_cue_points_postprocessed = m_points.size();

  m_id_timestamp_duration_multimap.clear();
}

uint64_t
cues_c::calculate_total_size()
  const {
  return std::accumulate(m_points.begin(), m_points.end(), 0ull, [this](uint64_t sum, cue_point_t const &point) { return sum + calculate_point_size(point); });
}

uint64_t
cues_c::calculate_bytes_for_uint(uint64_t value)
  const {
  for (int idx = 1; 7 >= idx; ++idx)
    if (value < (1ull << (idx * 8)))
      return idx;
  return 8;
}

uint64_t
cues_c::calculate_point_size(cue_point_t const &point)
  const {
  uint64_t point_size = EBML_ID(libmatroska::KaxCuePoint).GetLength()           + 1
                      + EBML_ID(libmatroska::KaxCuePoint).GetLength()           + 1 + calculate_bytes_for_uint(point.timestamp / g_timestamp_scale)
                      + EBML_ID(libmatroska::KaxCueTrackPositions).GetLength()  + 1
                      + EBML_ID(libmatroska::KaxCueTrack).GetLength()           + 1 + calculate_bytes_for_uint(point.track_num)
                      + EBML_ID(libmatroska::KaxCueClusterPosition).GetLength() + 1 + calculate_bytes_for_uint(point.cluster_position);

  auto codec_state_position = m_codec_state_position_map.find({ point.track_num, point.timestamp });
  if (codec_state_position != m_codec_state_position_map.end())
    point_size += EBML_ID(libmatroska::KaxCueCodecState).GetLength() + 1 + calculate_bytes_for_uint(codec_state_position->second);

  if (point.relative_position)
    point_size += EBML_ID(libmatroska::KaxCueRelativePosition).GetLength() + 1 + calculate_bytes_for_uint(point.relative_position);

  if (point.duration)
    point_size += EBML_ID(libmatroska::KaxCueDuration).GetLength() + 1 + calculate_bytes_for_uint(round_timestamp_scale(point.duration) / g_timestamp_scale);

  return point_size;
}

void
cues_c::adjust_positions(uint64_t old_position,
                         uint64_t delta) {
  auto s_debug_rerender_track_headers = debugging_option_c{"rerender|rerender_track_headers"};

  if (!delta || (m_points.empty() && m_codec_state_position_map.empty()))
    return;

  mxdebug_if(s_debug_rerender_track_headers,
             fmt::format("[rerender] cues_c::adjust_positions: old_position {0} delta {1} num_points {2} first point's position {3}\n",
                         old_position, delta, m_points.size(), !m_points.empty() ? m_points[0].cluster_position : 0));

  for (auto &point : m_points)
    if (point.cluster_position >= old_position)
      point.cluster_position += delta;

  for (auto &element : m_codec_state_position_map)
    if (element.second >= old_position)
      element.second += delta;
}

cues_c &
cues_c::get() {
  if (!s_cues)
    s_cues = std::make_shared<cues_c>();
  return *s_cues;
}
