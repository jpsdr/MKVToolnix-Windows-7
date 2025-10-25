/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   math helper functions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/math.h"

namespace mtx::math {

uint64_t
round_to_nearest_pow2(uint64_t value) {
  uint64_t best_value = 0;
  uint64_t test_value = 1;

  while (0x8000000000000000ull >= test_value) {
    if (  (value > test_value ? value - test_value : test_value - value)
        < (value > best_value ? value - best_value : best_value - value))
      best_value = test_value;
    test_value <<= 1;
  }

  return best_value;
}

int
int_log2(uint64_t value) {
  for (int bit = 63; bit >= 0; --bit)
    if (value & (1ull << bit))
      return bit;

  return -1;
}

double
int_to_double(int64_t value) {
  if (static_cast<uint64_t>(value + value) > (0xffeull << 52))
    return static_cast<double>(NAN);
  return std::ldexp(((value & ((1ll << 52) - 1)) + (1ll << 52)) * (value >> 63 | 1), (value >> 52 & 0x7ff) - 1075);
}

mtx_mp_rational_t
clamp_values_to(mtx_mp_rational_t const &r,
                int64_t max_value) {
  auto num = boost::multiprecision::numerator(r);
  auto den = boost::multiprecision::denominator(r);

  if (!num || !den || ((num <= max_value) && (den <= max_value)))
    return r;

  // 333 / 1000 , clamp = 500, mul = 1/2 = 1/(max/clamp) = clamp/max

  auto mult = mtx::rational(max_value, std::max(num, den));
  den       = mtx::to_int(den * mult);

  return mtx::rational(num * mult, den ? den : 1);
}
}
