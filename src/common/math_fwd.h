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

using mtx_mp_rational_t = boost::multiprecision::number<boost::multiprecision::backends::gmp_rational, boost::multiprecision::et_off>;
using mtx_mp_int_t      = boost::multiprecision::number<boost::multiprecision::backends::gmp_int,      boost::multiprecision::et_off>;

#if FMT_VERSION >= 90000
template <> struct fmt::formatter<mtx_mp_rational_t> : ostream_formatter {};
template <> struct fmt::formatter<mtx_mp_int_t>      : ostream_formatter {};
#endif  // FMT_VERSION >= 90000

namespace mtx {

// This conversion function exists to work around incomplete
// implementations of the constructors for
// boost::multiprecision::number with a backend of gmp_rational in
// older Boost versions.
template<typename Tnumerator,
         typename Tdenominator>
mtx_mp_rational_t
rational(Tnumerator &&numerator,
         Tdenominator &&denominator) {
  return mtx_mp_rational_t{std::forward<Tnumerator>(numerator)} / mtx_mp_rational_t{std::forward<Tdenominator>(denominator)};
}

// A simple static_cast<int64_t>(boost::multiprecision::mpq_rational)
// fails (yields std::numeric_limits<int64_t>::max()) on certain
// platforms, especially 32-bit ones. Conversion via
// boost::multiprecision::int128_t first seems to be a workaround. See
// https://github.com/boostorg/multiprecision/issues/342
inline int64_t
to_int(mtx_mp_rational_t const &value) {
  return static_cast<int64_t>(static_cast<boost::multiprecision::int128_t>(value));
}

inline uint64_t
to_uint(mtx_mp_rational_t const &value) {
  return static_cast<uint64_t>(static_cast<boost::multiprecision::uint128_t>(value));
}

// Additional conversions that round to the nearest integer away from 0.
inline int64_t
to_int_rounded(mtx_mp_rational_t const &value) {
  auto shift = value >= 0 ? 5 : -5;
  return static_cast<int64_t>((static_cast<boost::multiprecision::int128_t>(value * 10 + shift)) / 10);
}

inline uint64_t
to_uint_rounded(mtx_mp_rational_t const &value) {
  return static_cast<uint64_t>((static_cast<boost::multiprecision::uint128_t>(value * 10 + 5)) / 10);
}

namespace math {
}

}
