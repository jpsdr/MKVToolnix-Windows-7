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

#include "merge/timecode_calculator.h"
#include "merge/packet.h"

timecode_calculator_c::timecode_calculator_c(int64_t samples_per_second)
  : m_reference_timecode{timecode_c::ns(0)}
  , m_samples_per_second{samples_per_second}
  , m_samples_since_reference_timecode{}
  , m_samples_to_timecode{1000000000, static_cast<int64_t>(samples_per_second)}
{
}

void
timecode_calculator_c::add_timecode(timecode_c const &timecode) {
  if (timecode.valid())
    m_available_timecodes.push_back(timecode);
}

void
timecode_calculator_c::add_timecode(int64_t timecode) {
  if (-1 != timecode)
    m_available_timecodes.push_back(timecode_c::ns(timecode));
}

void
timecode_calculator_c::add_timecode(packet_cptr const &packet) {
  if (packet->has_timecode())
    m_available_timecodes.push_back(timecode_c::ns(packet->timecode));
}

timecode_c
timecode_calculator_c::get_next_timecode(int64_t samples_in_frame) {
  if (!m_available_timecodes.empty()) {
    auto next_timecode                 = m_available_timecodes.front();
    m_reference_timecode               = next_timecode;
    m_samples_since_reference_timecode = samples_in_frame;

    m_available_timecodes.pop_front();

    return next_timecode;
  }

  if (!m_samples_per_second)
    throw std::invalid_argument{"samples per second must not be 0"};

  auto next_timecode                  = m_reference_timecode + timecode_c::ns(m_samples_to_timecode * m_samples_since_reference_timecode);
  m_samples_since_reference_timecode += samples_in_frame;

  return next_timecode;
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
