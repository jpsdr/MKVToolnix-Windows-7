/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   math helper functions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef MTX_COMMON_MATH_H
#define MTX_COMMON_MATH_H

#include "common/common_pch.h"

#if defined(COMP_MSC)
# include <intrin.h>
#endif

namespace mtx { namespace math {

inline std::size_t
count_1_bits(uint64_t value) {
#if defined(COMP_MSC)
  return __popcnt(value);
#else
  return __builtin_popcountll(value);
#endif
}

inline int64_t
irnd(double a) {
  return a + 0.5;
}

uint64_t round_to_nearest_pow2(uint64_t value);
int int_log2(uint64_t value);
double int_to_double(int64_t value);

}}

using int64_rational_c = boost::rational<int64_t>;

#endif  // MTX_COMMON_MATH_H
