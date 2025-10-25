/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the samples-to-timestamp converter

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

class samples_to_timestamp_converter_c {
protected:
  int64_t m_numerator, m_denominator;

public:
  samples_to_timestamp_converter_c()
    : m_numerator(0)
    , m_denominator(0)
  { }

  samples_to_timestamp_converter_c(int64_t numerator, int64_t denominator)
    : m_numerator(0)
    , m_denominator(0)
  {
    set(numerator, denominator);
  }

  void set(int64_t numerator, int64_t denominator);

  inline int64_t operator *(int64_t v1) {
    return v1 * *this;
  }

  inline int64_t operator /(int64_t v1) {
    return v1 / *this;
  }

  friend int64_t operator *(int64_t v1, const samples_to_timestamp_converter_c &v2);
  friend int64_t operator /(int64_t v1, const samples_to_timestamp_converter_c &v2);
};

inline int64_t
operator *(int64_t v1,
           const samples_to_timestamp_converter_c &v2) {
  return v2.m_denominator ? v1 * v2.m_numerator / v2.m_denominator : v1;
}

inline int64_t
operator /(int64_t v1,
           const samples_to_timestamp_converter_c &v2) {
  return v2.m_numerator ? v1 * v2.m_denominator / v2.m_numerator : v1;
}
