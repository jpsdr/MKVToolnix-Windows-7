/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   the timestamp calculator implementation

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/strings/formatting.h"
#include "merge/timestamp_calculator.h"
#include "merge/packet.h"

timestamp_calculator_c::timestamp_calculator_c(int64_t samples_per_second)
  : m_reference_timestamp{timestamp_c::ns(0)}
  , m_samples_per_second{samples_per_second}
  , m_samples_since_reference_timestamp{}
  , m_samples_to_timestamp{1000000000, static_cast<int64_t>(samples_per_second)}
  , m_allow_smaller_timestamps{}
  , m_debug{"timestamp_calculator"}
{
}

void
timestamp_calculator_c::add_timestamp(timestamp_c const &timestamp,
                                      std::optional<uint64_t> stream_position) {
  if (!timestamp.valid())
    return;

  if (   (!m_last_timestamp_returned.valid() || (timestamp > m_last_timestamp_returned)           || (m_allow_smaller_timestamps && (timestamp < m_last_timestamp_returned)))
      && (m_available_timestamps.empty()     || (timestamp > m_available_timestamps.back().first) || (m_allow_smaller_timestamps && (timestamp < m_available_timestamps.back().first)))) {
    mxdebug_if(m_debug, fmt::format("timestamp_calculator::add_timestamp: adding {0}\n", mtx::string::format_timestamp(timestamp)));
    m_available_timestamps.emplace_back(timestamp, stream_position);

  } else
    mxdebug_if(m_debug, fmt::format("timestamp_calculator::add_timestamp: dropping {0}\n", mtx::string::format_timestamp(timestamp)));
}

void
timestamp_calculator_c::add_timestamp(int64_t timestamp,
                                      std::optional<uint64_t> stream_position) {
  if (0 <= timestamp)
    add_timestamp(timestamp_c::ns(timestamp), stream_position);
}

void
timestamp_calculator_c::add_timestamp(std::optional<int64_t> const &timestamp,
                                      std::optional<uint64_t> stream_position) {
  if (timestamp && (0 <= *timestamp))
    add_timestamp(timestamp_c::ns(*timestamp), stream_position);
}

void
timestamp_calculator_c::add_timestamp(packet_cptr const &packet,
                                      std::optional<uint64_t> stream_position) {
  if (packet->has_timestamp())
    add_timestamp(timestamp_c::ns(packet->timestamp), stream_position);
}

void
timestamp_calculator_c::drop_timestamps_before_position(uint64_t stream_position) {
  if (m_available_timestamps.empty())
    return;

  auto itr = std::find_if(m_available_timestamps.begin(), m_available_timestamps.end(), [stream_position](auto const &entry) {
    return entry.second && (*entry.second >= stream_position);
  });

  if (itr != m_available_timestamps.end())
    m_available_timestamps.erase(m_available_timestamps.begin(), itr);
}

timestamp_c
timestamp_calculator_c::get_next_timestamp(int64_t samples_in_frame,
                                           std::optional<uint64_t> stream_position) {
  if (   !m_available_timestamps.empty()
      && (   !stream_position
          || !m_available_timestamps.front().second
          || (*m_available_timestamps.front().second <= *stream_position)))
    return fetch_next_available_timestamp(samples_in_frame);

  return calculate_next_timestamp(samples_in_frame);
}

timestamp_c
timestamp_calculator_c::fetch_next_available_timestamp(int64_t samples_in_frame) {
  m_last_timestamp_returned           = m_available_timestamps.front().first;
  m_reference_timestamp               = m_last_timestamp_returned;
  m_samples_since_reference_timestamp = samples_in_frame;

  m_available_timestamps.pop_front();

  mxdebug_if(m_debug, fmt::format("timestamp_calculator_c::get_next_timestamp: returning available {0}\n", mtx::string::format_timestamp(m_last_timestamp_returned)));

  return m_last_timestamp_returned;
}

timestamp_c
timestamp_calculator_c::calculate_next_timestamp(int64_t samples_in_frame) {
  if (!m_samples_per_second)
    throw std::invalid_argument{"samples per second must not be 0"};

  m_last_timestamp_returned           = m_reference_timestamp + timestamp_c::ns(m_samples_to_timestamp * m_samples_since_reference_timestamp);
  m_samples_since_reference_timestamp += samples_in_frame;

  mxdebug_if(m_debug, fmt::format("timestamp_calculator_c::get_next_timestamp: returning calculated {0}\n", mtx::string::format_timestamp(m_last_timestamp_returned)));

  return m_last_timestamp_returned;
}

timestamp_c
timestamp_calculator_c::get_duration(int64_t samples) {
  if (!m_samples_per_second)
    throw std::invalid_argument{"samples per second must not be 0"};

  return timestamp_c::ns(m_samples_to_timestamp * samples);
}

void
timestamp_calculator_c::set_samples_per_second(int64_t samples_per_second) {
  if (!samples_per_second || (samples_per_second == m_samples_per_second))
    return;

  m_reference_timestamp               += get_duration(m_samples_since_reference_timestamp);
  m_samples_since_reference_timestamp  = 0;
  m_samples_to_timestamp.set(1000000000, samples_per_second);
}

void
timestamp_calculator_c::set_allow_smaller_timestamps(bool allow) {
  m_allow_smaller_timestamps = allow;
}
