/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   math helper functions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

using mtx_mp_rational_t = boost::multiprecision::number<boost::multiprecision::gmp_rational, boost::multiprecision::et_off>;
using mtx_mp_int_t      = boost::multiprecision::number<boost::multiprecision::gmp_int,      boost::multiprecision::et_off>;

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
#if BOOST_VERSION >= 107100
  return mtx_mp_rational_t{std::forward<Tnumerator>(numerator), std::forward<Tdenominator>(denominator)};
#else
  return mtx_mp_rational_t{std::forward<Tnumerator>(numerator)} / mtx_mp_rational_t{std::forward<Tdenominator>(denominator)};
#endif
}

namespace math {
}

}
