/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   math helper functions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#if defined(COMP_MSC)
# include <intrin.h>
#endif

#include "common/math_fwd.h"

namespace mtx::math {

inline std::size_t
count_1_bits(uint64_t value) {
#if defined(COMP_MSC)
  return __popcnt(value);
#else
  return __builtin_popcountll(value);
#endif
}

uint64_t round_to_nearest_pow2(uint64_t value);
int int_log2(uint64_t value);
double int_to_double(int64_t value);
mtx_mp_rational_t clamp_values_to(mtx_mp_rational_t const &r, int64_t max_value);

// Converting unsigned int types to signed ints assuming the
// underlying bits in memory should represent the 2's complement of a
// signed integer. See https://stackoverflow.com/a/13208789/507077

template<typename Tunsigned>
typename std::enable_if<
  std::is_unsigned<Tunsigned>::value,
  typename std::make_signed<Tunsigned>::type
>::type
to_signed(Tunsigned const &u) {
  using Tsigned = typename std::make_signed<Tunsigned>::type;

  if (u <= static_cast<Tunsigned>(std::numeric_limits<Tsigned>::max()))
    return static_cast<Tsigned>(u);

  return static_cast<Tsigned>(u - std::numeric_limits<Tsigned>::min()) + std::numeric_limits<Tsigned>::min();
}

template<typename Tsigned>
typename std::enable_if<
  std::is_signed<Tsigned>::value,
  Tsigned
>::type
to_signed(Tsigned const &s) {
  return s;
}

}
