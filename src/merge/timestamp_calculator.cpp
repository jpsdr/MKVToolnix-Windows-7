/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   the timestamp calculator implementation

   Written by Moritz Bunkus <moritz@bunkus.org>.
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
  , m_debug{"timestamp_calculator"}
{
}

void
timestamp_calculator_c::add_timestamp(timestamp_c const &timestamp) {
  if (!timestamp.valid())
    return;

  if (   (!m_last_timestamp_returned.valid() || (timestamp > m_last_timestamp_returned))
      && (m_available_timestamps.empty()     || (timestamp > m_available_timestamps.back()))) {
    mxdebug_if(m_debug, boost::format("timestamp_calculator::add_timestamp: adding %1%\n") % format_timestamp(timestamp));
    m_available_timestamps.push_back(timestamp);

  } else
    mxdebug_if(m_debug, boost::format("timestamp_calculator::add_timestamp: dropping %1%\n") % format_timestamp(timestamp));
}

void
timestamp_calculator_c::add_timestamp(int64_t timestamp) {
  if (-1 != timestamp)
    add_timestamp(timestamp_c::ns(timestamp));
}

void
timestamp_calculator_c::add_timestamp(packet_cptr const &packet) {
  if (packet->has_timecode())
    add_timestamp(timestamp_c::ns(packet->timecode));
}

timestamp_c
timestamp_calculator_c::get_next_timestamp(int64_t samples_in_frame) {
  if (!m_available_timestamps.empty()) {
    m_last_timestamp_returned           = m_available_timestamps.front();
    m_reference_timestamp               = m_last_timestamp_returned;
    m_samples_since_reference_timestamp = samples_in_frame;

    m_available_timestamps.pop_front();

    mxdebug_if(m_debug, boost::format("timestamp_calculator_c::get_next_timestamp: returning available %1%\n") % format_timestamp(m_last_timestamp_returned));

    return m_last_timestamp_returned;
  }

  if (!m_samples_per_second)
    throw std::invalid_argument{"samples per second must not be 0"};

  m_last_timestamp_returned           = m_reference_timestamp + timestamp_c::ns(m_samples_to_timestamp * m_samples_since_reference_timestamp);
  m_samples_since_reference_timestamp += samples_in_frame;

  mxdebug_if(m_debug, boost::format("timestamp_calculator_c::get_next_timestamp: returning calculated %1%\n") % format_timestamp(m_last_timestamp_returned));

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
