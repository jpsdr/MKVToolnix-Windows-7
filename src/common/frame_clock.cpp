/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   a clock for frame timestamps counted in units of an exact length
*/

#include "common/common_pch.h"

#include "common/frame_clock.h"

namespace mtx {

void
frame_clock_c::set_unit(int64_t num,
                        int64_t den) {
  set_unit(mtx::rational(num, den));
}

void
frame_clock_c::set_unit(mtx_mp_rational_t const &seconds) {
  auto unit_length = seconds * 1'000'000'000;
  if (unit_length == m_unit_length)
    return;

  m_anchor      = now();
  m_units       = 0;
  m_unit_length = unit_length;
}

bool
frame_clock_c::has_unit()
  const {
  return m_unit_length != 0;
}

void
frame_clock_c::reset(int64_t anchor) {
  m_anchor = anchor;
  m_units  = 0;
}

int64_t
frame_clock_c::now()
  const {
  return peek(0);
}

int64_t
frame_clock_c::peek(int64_t units)
  const {
  return m_anchor + mtx::to_int_rounded(m_unit_length * (m_units + units));
}

int64_t
frame_clock_c::advance(int64_t units) {
  auto timestamp  = now();
  m_units        += units;
  return timestamp;
}

}
