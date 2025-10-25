/** AVC & HEVC ES parser base class

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   \author Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/checksums/base_fwd.h"
#include "common/endian.h"
#include "common/memory_slice_cursor.h"
#include "common/mm_file_io.h"
#include "common/mpeg.h"
#include "common/strings/formatting.h"
#include "common/xyzvc/es_parser.h"

namespace mtx::xyzvc {

es_parser_c::es_parser_c(std::string const &debug_type,
                         std::size_t num_slice_types,
                         std::size_t num_nalu_types)
  : m_debug_type{debug_type}
  , m_debug_keyframe_detection{fmt::format("{0}_parser|{0}_keyframe_detection", debug_type, debug_type)}
  , m_debug_nalu_types{        fmt::format("{0}_parser|{0}_nalu_types",         debug_type, debug_type)}
  , m_debug_timestamps{        fmt::format("{0}_parser|{0}_timestamps",         debug_type, debug_type)}
  , m_debug_sps_info{          fmt::format("{0}_parser|{0}_sps|{0}_sps_info",   debug_type, debug_type, debug_type)}
  , m_debug_statistics{        fmt::format("{0}_parser|{0}_statistics",         debug_type, debug_type)}
  , m_stats{num_slice_types, num_nalu_types}
{
}

es_parser_c::~es_parser_c() {
  debug_dump_statistics();
}

void
es_parser_c::discard_actual_frames(bool discard) {
  m_discard_actual_frames = discard;
}

void
es_parser_c::set_next_i_slice_is_key_frame() {
  m_recovery_point_valid = true;
}

void
es_parser_c::set_normalize_parameter_sets(bool normalize) {
  m_normalize_parameter_sets = normalize;
}

void
es_parser_c::maybe_dump_raw_data(uint8_t const *buffer,
                                 std::size_t size) {
  static debugging_option_c s_dump_raw_data{fmt::format("{0}_es_parser_dump_raw_data", m_debug_type)};

  if (!s_dump_raw_data)
    return;

  auto file_name = fmt::format("{0}_raw_data-{1:p}", m_debug_type, static_cast<void *>(this));
  mm_file_io_c out{file_name, boost::filesystem::is_regular_file(file_name) ? libebml::MODE_WRITE : libebml::MODE_CREATE};

  out.setFilePointer(0, libebml::seek_end);
  out.write(buffer, size);
}

void
es_parser_c::add_bytes(memory_cptr const &buf) {
  add_bytes(buf->get_buffer(), buf->get_size());
}

void
es_parser_c::add_bytes(uint8_t *buffer,
                       std::size_t size) {
  maybe_dump_raw_data(buffer, size);

  mtx::mem::slice_cursor_c cursor;
  int marker_size              = 0;
  int previous_marker_size     = 0;
  int previous_pos             = -1;
  uint64_t previous_parsed_pos = m_parsed_position;

  if (m_unparsed_buffer && (0 != m_unparsed_buffer->get_size()))
    cursor.add_slice(m_unparsed_buffer);
  cursor.add_slice(buffer, size);

  if (3 <= cursor.get_remaining_size()) {
    uint32_t marker =                               1 << 24
                    | (unsigned int)cursor.get_char() << 16
                    | (unsigned int)cursor.get_char() <<  8
                    | (unsigned int)cursor.get_char();

    while (true) {
      if (NALU_START_CODE == marker)
        marker_size = 4;
      else if (NALU_START_CODE == (marker & 0x00ffffff))
        marker_size = 3;

      if (0 != marker_size) {
        if (-1 != previous_pos) {
          int new_size = cursor.get_position() - marker_size - previous_pos - previous_marker_size;
          auto nalu = memory_c::alloc(new_size);
          cursor.copy(nalu->get_buffer(), previous_pos + previous_marker_size, new_size);
          m_parsed_position = previous_parsed_pos + previous_pos;

          mtx::mpeg::remove_trailing_zero_bytes(*nalu);
          if (nalu->get_size())
            handle_nalu(nalu, m_parsed_position);
        }
        previous_pos         = cursor.get_position() - marker_size;
        previous_marker_size = marker_size;
        marker_size          = 0;
      }

      if (!cursor.char_available())
        break;

      marker <<= 8;
      marker  |= (unsigned int)cursor.get_char();
    }
  }

  if (-1 == previous_pos)
    previous_pos = 0;

  m_stream_position += size;
  m_parsed_position  = previous_parsed_pos + previous_pos;

  int new_size = cursor.get_size() - previous_pos;
  if (0 != new_size) {
    m_unparsed_buffer = memory_c::alloc(new_size);
    cursor.copy(m_unparsed_buffer->get_buffer(), previous_pos, new_size);

  } else
    m_unparsed_buffer.reset();
}

void
es_parser_c::add_bytes_framed(memory_cptr const &buf,
                              std::size_t nalu_size_length) {
  add_bytes_framed(buf->get_buffer(), buf->get_size(), nalu_size_length);
}

void
es_parser_c::add_bytes_framed(uint8_t *buffer,
                              std::size_t buffer_size,
                              std::size_t nalu_size_length) {
  maybe_dump_raw_data(buffer, buffer_size);

  auto pos = buffer;
  auto end = buffer + buffer_size;

  while ((pos + nalu_size_length) <= end) {
    auto nalu_size     = get_uint_be(pos, nalu_size_length);
    pos               += nalu_size_length;
    m_stream_position += nalu_size_length;

    if (!nalu_size)
      continue;

    if ((pos + nalu_size) > end)
      return;

    handle_nalu(memory_c::borrow(pos, nalu_size), m_stream_position);

    pos               += nalu_size;
    m_stream_position += nalu_size;
  }
}

void
es_parser_c::force_default_duration(int64_t default_duration) {
  m_forced_default_duration = default_duration;
}

bool
es_parser_c::is_default_duration_forced()
  const {
  return -1 != m_forced_default_duration;
}

void
es_parser_c::set_container_default_duration(int64_t default_duration) {
  m_container_default_duration = default_duration;
}

bool
es_parser_c::has_stream_default_duration()
  const {
  return -1 != m_stream_default_duration;
}

int64_t
es_parser_c::get_stream_default_duration()
  const {
  assert(-1 != m_stream_default_duration);
  return m_stream_default_duration;
}

void
es_parser_c::set_keep_ar_info(bool keep) {
  m_keep_ar_info = keep;
}

void
es_parser_c::set_nalu_size_length(int nalu_size_length) {
  m_nalu_size_length = nalu_size_length;
}

int
es_parser_c::get_nalu_size_length()
  const {
  return m_nalu_size_length;
}

bool
es_parser_c::frame_available()
  const {
  return !m_frames_out.empty();
}

std::size_t
es_parser_c::get_num_frames_available()
  const {
  return m_frames_out.size();
}

mtx::xyzvc::frame_t
es_parser_c::get_frame() {
  assert(!m_frames_out.empty());

  auto frame = *m_frames_out.begin();
  m_frames_out.erase(m_frames_out.begin(), m_frames_out.begin() + 1);

  frame.combine_nalus_to_data(m_nalu_size_length);

  return frame;
}

bool
es_parser_c::configuration_record_changed()
  const {
  return m_configuration_record_changed;
}

void
es_parser_c::add_timestamp(int64_t timestamp) {
  m_provided_timestamps.emplace_back(timestamp, m_stream_position);
  ++m_stats.num_timestamps_in;
}

void
es_parser_c::add_nalu_to_extra_data(memory_cptr const &nalu,
                                    extra_data_position_e position) {
  if (position == extra_data_position_e::dont_store)
    return;

  nalu->take_ownership();

  auto &container = position == extra_data_position_e::pre  ? m_extra_data_pre : m_extra_data_initial;
  container.push_back(nalu);
}

void
es_parser_c::add_nalu_to_pending_frame_data(memory_cptr const &nalu) {
  nalu->take_ownership();
  m_pending_frame_data.emplace_back(nalu);
}

void
es_parser_c::add_parameter_sets_to_extra_data() {
  std::unordered_map<uint32_t, bool> is_in_extra_data;

  for (auto const &data : m_extra_data_pre) {
    if (does_nalu_get_included_in_extra_data(*data))
      return;

    is_in_extra_data[mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, *data)] = true;
  }

  auto old_extra_data = std::move(m_extra_data_pre);

  m_extra_data_pre.clear();
  m_extra_data_pre.reserve(m_vps_list.size() + m_sps_list.size() + m_pps_list.size() + old_extra_data.size() + m_extra_data_initial.size());

  auto inserter = std::back_inserter(m_extra_data_pre);

  std::copy(m_vps_list.begin(), m_vps_list.end(), inserter);
  std::copy(m_sps_list.begin(), m_sps_list.end(), inserter);
  std::copy(m_pps_list.begin(), m_pps_list.end(), inserter);

  for (auto const &data : m_extra_data_initial)
    if (!is_in_extra_data[mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, *data)])
      inserter = data;

  std::copy(old_extra_data.begin(), old_extra_data.end(), inserter);

  m_extra_data_initial.clear();
}

void
es_parser_c::build_frame_data() {
  if (m_incomplete_frame.m_keyframe && m_normalize_parameter_sets)
    add_parameter_sets_to_extra_data();

  m_incomplete_frame.m_data_parts = std::move(m_extra_data_pre);
  m_incomplete_frame.m_data_parts.reserve(m_incomplete_frame.m_data_parts.size() + m_pending_frame_data.size());

  std::copy(m_pending_frame_data.begin(), m_pending_frame_data.end(), std::back_inserter(m_incomplete_frame.m_data_parts));

  m_extra_data_pre.clear();
  m_pending_frame_data.clear();
}

void
es_parser_c::add_nalu_to_unhandled_nalus(memory_cptr const &nalu,
                                         uint64_t nalu_pos) {
  nalu->take_ownership();
  m_unhandled_nalus.emplace_back(nalu, nalu_pos);
}

void
es_parser_c::flush_unhandled_nalus() {
  if (m_unhandled_nalus.empty())
    return;

  mxdebug_if(m_debug_nalu_types, fmt::format("flushing {0} unhandled NALUs\n", m_unhandled_nalus.size()));

  for (auto const &nalu_with_pos : m_unhandled_nalus)
    handle_nalu(nalu_with_pos.first, nalu_with_pos.second);

  m_unhandled_nalus.clear();
}

int
es_parser_c::get_num_skipped_frames()
  const {
  return m_num_skipped_frames;
}

int64_t
es_parser_c::get_most_often_used_duration()
  const {
  int64_t const s_common_default_durations[] = {
    1000000000ll / 50,
    1000000000ll / 25,
    1000000000ll / 60,
    1000000000ll / 30,
    1000000000ll * 1001 / 48000,
    1000000000ll * 1001 / 24000,
    1000000000ll * 1001 / 60000,
    1000000000ll * 1001 / 30000,
  };

  auto most_often = m_duration_frequency.begin();
  for (auto current = m_duration_frequency.begin(); m_duration_frequency.end() != current; ++current)
    if (current->second > most_often->second)
      most_often = current;

  // No duration at all!? No frame?
  if (m_duration_frequency.end() == most_often) {
    mxdebug_if(m_debug_timestamps, fmt::format("Duration frequency: none found, using 25 FPS\n"));
    return 1000000000ll / 25;
  }

  auto best = std::make_pair(most_often->first, std::numeric_limits<uint64_t>::max());

  for (auto common_default_duration : s_common_default_durations) {
    uint64_t diff = std::abs(most_often->first - common_default_duration);
    if ((diff < 20000) && (diff < best.second))
      best = std::make_pair(common_default_duration, diff);
  }

  mxdebug_if(m_debug_timestamps,
             fmt::format("Duration frequency. Result: {0}, diff {1}. Best before adjustment: {2}. All: {3}\n",
                         best.first, best.second, most_often->first,
                         std::accumulate(m_duration_frequency.begin(), m_duration_frequency.end(), ""s, [](auto const &accu, auto const &pair) {
                           return accu + fmt::format(" <{0} {1}>", pair.first, pair.second);
                         })));

  return best.first;
}

std::vector<int64_t>
es_parser_c::calculate_provided_timestamps_to_use() {
  auto frame_idx                     = 0u;
  auto provided_timestamps_idx       = 0u;
  auto const num_frames              = m_frames.size();
  auto const num_provided_timestamps = m_provided_timestamps.size();

  std::vector<int64_t> provided_timestamps_to_use;
  provided_timestamps_to_use.reserve(num_frames);

  while (   (frame_idx               < num_frames)
         && (provided_timestamps_idx < num_provided_timestamps)) {
    timestamp_c timestamp_to_use;
    auto &frame = m_frames[frame_idx];

    while (   (provided_timestamps_idx < num_provided_timestamps)
           && (frame.m_position >= m_provided_timestamps[provided_timestamps_idx].second)) {
      timestamp_to_use = timestamp_c::ns(m_provided_timestamps[provided_timestamps_idx].first);
      ++provided_timestamps_idx;
    }

    if (timestamp_to_use.valid()) {
      provided_timestamps_to_use.emplace_back(timestamp_to_use.to_ns());
      frame.m_has_provided_timestamp = true;
    }

    ++frame_idx;
  }

  // mxdebug_if(m_debug_timestamps,
  //            fmt::format("cleanup; num frames {0} num provided timestamps available {1} num provided timestamps to use {2}\n"
  //                        "  frames:\n{3}"
  //                        "  provided timestamps (available):\n{4}"
  //                        "  provided timestamps (to use):\n{5}",
  //                        num_frames, num_provided_timestamps, provided_timestamps_to_use.size(),
  //                        std::accumulate(m_frames.begin(), m_frames.end(), std::string{}, [](auto const &str, auto const &frame) {
  //                          return str + fmt::format("    pos {0} size {1} type {2}\n", frame.m_position, frame.get_data_size(), frame.m_type);
  //                        }),
  //                        std::accumulate(m_provided_timestamps.begin(), m_provided_timestamps.end(), std::string{}, [](auto const &str, auto const &provided_timestamp) {
  //                          return str + fmt::format("    pos {0} timestamp {1}\n", provided_timestamp.second, mtx::string::format_timestamp(provided_timestamp.first));
  //                        }),
  //                        std::accumulate(provided_timestamps_to_use.begin(), provided_timestamps_to_use.end(), std::string{}, [](auto const &str, auto const &provided_timestamp) {
  //                          return str + fmt::format("    timestamp {0}\n", mtx::string::format_timestamp(provided_timestamp));
  //                        })));

  m_provided_timestamps.erase(m_provided_timestamps.begin(), m_provided_timestamps.begin() + provided_timestamps_idx);

  std::sort(provided_timestamps_to_use.begin(), provided_timestamps_to_use.end());

  return provided_timestamps_to_use;
}

void
es_parser_c::calculate_frame_timestamps(std::vector<int64_t> const &provided_timestamps_to_use) {
  auto frames_begin           = m_frames.begin();
  auto frames_end             = m_frames.end();
  auto previous_frame_itr     = frames_begin;
  auto provided_timestamp_itr = provided_timestamps_to_use.begin();

  for (auto frame_itr = frames_begin; frames_end != frame_itr; ++frame_itr) {
    if (frame_itr->m_has_provided_timestamp) {
      frame_itr->m_start = *provided_timestamp_itr;
      ++provided_timestamp_itr;

      if (frames_begin != frame_itr)
        previous_frame_itr->m_end = frame_itr->m_start;

    } else {
      frame_itr->m_start = frames_begin == frame_itr ? m_max_timestamp : previous_frame_itr->m_end;
      ++m_stats.num_timestamps_generated;
    }

    frame_itr->m_end = frame_itr->m_start + duration_for(frame_itr->m_si);

    previous_frame_itr = frame_itr;
  }

  m_max_timestamp = m_frames.back().m_end;
}

void
es_parser_c::calculate_frame_references() {
  for (auto frame_itr = m_frames.begin(), frames_end = m_frames.end(); frames_end != frame_itr; ++frame_itr) {
    if (frame_itr->is_i_frame()) {
      m_previous_i_p_start = frame_itr->m_start;
      continue;
    }

    frame_itr->m_ref1 = m_previous_i_p_start - frame_itr->m_start;

    if (frame_itr->is_p_frame()) {
      m_previous_i_p_start = frame_itr->m_start;
      continue;
    }

    auto next_i_p_frame_itr = frame_itr + 1;

    while ((frames_end != next_i_p_frame_itr) && next_i_p_frame_itr->is_b_frame())
      ++next_i_p_frame_itr;

    auto forward_ref_start = frames_end != next_i_p_frame_itr ? next_i_p_frame_itr->m_start : m_max_timestamp;
    frame_itr->m_ref2      = forward_ref_start - frame_itr->m_start;
  }
}

void
es_parser_c::update_frame_stats() {
  mxdebug_if(m_debug_timestamps, fmt::format("DECODE order dump\n"));

  for (auto &frame : m_frames) {
    mxdebug_if(m_debug_timestamps, fmt::format("  type {0} TS {1} size {2} pos 0x{3:x} ref1 {4} ref2 {5}\n", frame.m_type, mtx::string::format_timestamp(frame.m_start), frame.get_data_size(), frame.m_position, frame.m_ref1, frame.m_ref2));

    ++m_duration_frequency[frame.m_end - frame.m_start];

    if (frame.m_si.field_pic_flag)
      ++m_stats.num_field_slices;
    else
      ++m_stats.num_frame_slices;
  }
}

void
es_parser_c::cleanup(std::deque<frame_t> &queue) {
  auto num_frames = m_frames.size();
  if (!num_frames)
    return;

  if (m_discard_actual_frames) {
    m_stats.num_frames_discarded    += m_frames.size();
    m_stats.num_timestamps_discarded += m_provided_timestamps.size();

    m_frames.clear();
    m_provided_timestamps.clear();

    return;
  }

  calculate_frame_order();

  auto provided_timestamps_to_use = calculate_provided_timestamps_to_use();

  if (!m_simple_picture_order)
    std::sort(m_frames.begin(), m_frames.end(), [](auto const &f1, auto const &f2) { return f1.m_presentation_order < f2.m_presentation_order; });

  calculate_frame_timestamps(provided_timestamps_to_use);
  calculate_frame_references();

  if (!m_simple_picture_order)
    std::sort(m_frames.begin(), m_frames.end(), [](auto const &f1, auto const &f2) { return f1.m_decode_order < f2.m_decode_order; });

  update_frame_stats();

  if (m_first_cleanup && !m_frames.front().m_keyframe) {
    // Drop all frames before the first key frames as they cannot be
    // decoded anyway.
    m_stats.num_frames_discarded += m_frames.size();
    m_frames.clear();

    return;
  }

  m_first_cleanup         = false;
  m_stats.num_frames_out += m_frames.size();
  queue.insert(queue.end(), m_frames.begin(), m_frames.end());
  m_frames.clear();
}

bool
es_parser_c::has_par_been_found()
  const {
  assert(m_configuration_record_ready);
  return m_par_found;
}

mtx_mp_rational_t const &
es_parser_c::get_par()
  const {
  assert(m_configuration_record_ready && m_par_found);
  return m_par;
}

std::pair<int64_t, int64_t> const
es_parser_c::get_display_dimensions(int width,
                                    int height)
  const {
  assert(m_configuration_record_ready && m_par_found);

  if (0 >= width)
    width = get_width();
  if (0 >= height)
    height = get_height();

  return std::make_pair<int64_t, int64_t>(1 <= m_par ? mtx::to_int_rounded(width * m_par) : width,
                                          1 <= m_par ? height                             : mtx::to_int_rounded(height / m_par));
}

size_t
es_parser_c::get_num_field_slices()
  const {
  return m_stats.num_field_slices;
}

size_t
es_parser_c::get_num_frame_slices()
  const {
  return m_stats.num_frame_slices;
}

std::string
es_parser_c::get_nalu_type_name(int type)
  const {
  auto name = m_nalu_names_by_type->find(type);
  return (m_nalu_names_by_type->end() == name) ? "unknown" : name->second;
}

void
es_parser_c::dump_info()
  const {
  auto dump_ps = [](std::string const &type, std::vector<memory_cptr> const &buffers) {
    mxinfo(fmt::format("Dumping {0}:\n", type));
    for (int idx = 0, num_entries = buffers.size(); idx < num_entries; ++idx)
      mxinfo(fmt::format("  {0} size {1} adler32 0x{2:08x}\n", idx, buffers[idx]->get_size(), mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, *buffers[idx])));
  };

  dump_ps("m_vps",                m_vps_list);
  dump_ps("m_sps",                m_sps_list);
  dump_ps("m_pps_list",           m_pps_list);
  dump_ps("m_extra_data_pre",     m_extra_data_pre);
  dump_ps("m_extra_data_initial", m_extra_data_initial);
  dump_ps("m_pending_frame_data", m_pending_frame_data);

  mxinfo("Dumping m_frames_out:\n");
  for (auto &frame : m_frames_out) {
    mxinfo(fmt::format("  size {0} key {1} start {2} end {3} ref1 {4} adler32 0x{5:08x}\n",
                       frame.get_data_size(),
                       frame.m_keyframe,
                       mtx::string::format_timestamp(frame.m_start),
                       mtx::string::format_timestamp(frame.m_end),
                       mtx::string::format_timestamp(frame.m_ref1),
                       frame.m_data ? mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, *frame.m_data) : 0u));
  }
}

void
es_parser_c::debug_dump_statistics()
  const {
  mxdebug_if(m_debug_timestamps, fmt::format("{0}: stream_position {1} parsed_position {2}\n", m_debug_type, m_stream_position, m_parsed_position));

  if (!m_debug_statistics)
    return;

  mxdebug(fmt::format("{0} statistics: #frames: out {1} discarded {2} #timestamps: in {3} generated {4} discarded {5} num_fields: {6} num_frames: {7} num_sei_nalus: {8} num_idr_slices: {9}\n",
                      m_debug_type,
                      m_stats.num_frames_out,   m_stats.num_frames_discarded, m_stats.num_timestamps_in, m_stats.num_timestamps_generated, m_stats.num_timestamps_discarded,
                      m_stats.num_field_slices, m_stats.num_frame_slices,     m_stats.num_sei_nalus,     m_stats.num_idr_slices));

  mxdebug(fmt::format("{0}: Number of NALUs by type:\n", m_debug_type));
  for (int i = 0, size = m_stats.num_nalus_by_type.size(); i < size; ++i)
    if (0 != m_stats.num_nalus_by_type[i])
      mxdebug(fmt::format("  {0}: {1}\n", get_nalu_type_name(i + 1), m_stats.num_nalus_by_type[i]));

  mxdebug(fmt::format("{0}: Number of slices by type:\n", m_debug_type));
  for (int i = 0, size = m_stats.num_slices_by_type.size(); i < size; ++i)
    if (0 != m_stats.num_slices_by_type[i])
      mxdebug(fmt::format("  {0}: {1}\n", i < static_cast<int>(m_slice_names_by_type->size()) ? m_slice_names_by_type->at(i) : "?"s, m_stats.num_slices_by_type[i]));
}

} // namespace mtx::xyzvc
