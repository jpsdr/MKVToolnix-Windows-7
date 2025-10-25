/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/math.h"
#include "common/samples_to_timestamp_converter.h"

void
samples_to_timestamp_converter_c::set(int64_t numerator,
                                      int64_t denominator) {
  if (0 == denominator)
    return;

  int64_t gcd   = std::gcd(numerator, denominator);

  m_numerator   = numerator   / gcd;
  m_denominator = denominator / gcd;
}
