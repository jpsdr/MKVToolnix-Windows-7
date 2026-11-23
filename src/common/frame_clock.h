/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   a clock for frame timestamps counted in units of an exact length
*/

#pragma once

#include "common/common_pch.h"

#include "common/math_fwd.h"

namespace mtx {

// Frame timestamps counted from an anchor in whole units of an exact length,
// e.g. one field of 1001/48000 s. Each timestamp is worked out from the count
// and rounded to the nearest ns once. Lengths cut to whole ns are never added
// up, so the timestamps do not drift.
class frame_clock_c {
protected:
  int64_t m_anchor{};               // ns
  int64_t m_units{};                // counted since the anchor
  mtx_mp_rational_t m_unit_length;  // ns, exact; 0: not set yet

public:
  // The length of one unit: num / den seconds. A different length restarts
  // the count at the current timestamp.
  void set_unit(int64_t num, int64_t den);
  void set_unit(mtx_mp_rational_t const &seconds);
  bool has_unit() const;

  // A timestamp given by the container: the count restarts there.
  void reset(int64_t anchor);

  // The current timestamp.
  int64_t now() const;
  // The timestamp `units` units after the current one.
  int64_t peek(int64_t units) const;
  // Moves `units` units on and returns the timestamp before the move.
  int64_t advance(int64_t units);
};

}
