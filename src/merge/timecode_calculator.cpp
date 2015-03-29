/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   the timecode calculator implementation

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/strings/formatting.h"
#include "merge/timecode_calculator.h"
#include "merge/packet.h"

timecode_calculator_c::timecode_calculator_c(int64_t samples_per_second)
  : m_reference_timecode{timecode_c::ns(0)}
  , m_samples_per_second{samples_per_second}
  , m_samples_since_reference_timecode{}
  , m_samples_to_timecode{1000000000, static_cast<int64_t>(samples_per_second)}
  , m_debug{"timecode_calculator"}
{
}

void
timecode_calculator_c::add_timecode(timecode_c const &timecode) {
  if (!timecode.valid())
    return;

  if (   (!m_last_timecode_returned.valid() || (timecode > m_last_timecode_returned))
      && (m_available_timecodes.empty()     || (timecode > m_available_timecodes.back()))) {
    mxdebug_if(m_debug, boost::format("timecode_calculator::add_timecode: adding %1%\n") % format_timecode(timecode));
    m_available_timecodes.push_back(timecode);

  } else
    mxdebug_if(m_debug, boost::format("timecode_calculator::add_timecode: dropping %1%\n") % format_timecode(timecode));
}

void
timecode_calculator_c::add_timecode(int64_t timecode) {
  if (-1 != timecode)
    add_timecode(timecode_c::ns(timecode));
}

void
timecode_calculator_c::add_timecode(packet_cptr const &packet) {
  if (packet->has_timecode())
    add_timecode(timecode_c::ns(packet->timecode));
}

timecode_c
timecode_calculator_c::get_next_timecode(int64_t samples_in_frame) {
  if (!m_available_timecodes.empty()) {
    m_last_timecode_returned           = m_available_timecodes.front();
    m_reference_timecode               = m_last_timecode_returned;
    m_samples_since_reference_timecode = samples_in_frame;

    m_available_timecodes.pop_front();

    mxdebug_if(m_debug, boost::format("timecode_calculator_c::get_next_timecode: returning available %1%\n") % format_timecode(m_last_timecode_returned));

    return m_last_timecode_returned;
  }

  if (!m_samples_per_second)
    throw std::invalid_argument{"samples per second must not be 0"};

  m_last_timecode_returned           = m_reference_timecode + timecode_c::ns(m_samples_to_timecode * m_samples_since_reference_timecode);
  m_samples_since_reference_timecode += samples_in_frame;

  mxdebug_if(m_debug, boost::format("timecode_calculator_c::get_next_timecode: returning calculated %1%\n") % format_timecode(m_last_timecode_returned));

  return m_last_timecode_returned;
}

timecode_c
timecode_calculator_c::get_duration(int64_t samples) {
  if (!m_samples_per_second)
    throw std::invalid_argument{"samples per second must not be 0"};

  return timecode_c::ns(m_samples_to_timecode * samples);
}

void
timecode_calculator_c::set_samples_per_second(int64_t samples_per_second) {
  if (!samples_per_second || (samples_per_second == m_samples_per_second))
    return;

  m_reference_timecode               += get_duration(m_samples_since_reference_timecode);
  m_samples_since_reference_timecode  = 0;
  m_samples_to_timecode.set(1000000000, samples_per_second);
}
