/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the basic_timestamp_c template class

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <boost/operators.hpp>
#include <stdexcept>

template<typename T>
class basic_timestamp_c
  : boost::totally_ordered< basic_timestamp_c<T>
  , boost::arithmetic< basic_timestamp_c<T>
  > >
{
private:
  T m_timestamp;
  bool m_valid;

  explicit basic_timestamp_c(T timestamp)
    : m_timestamp{timestamp}
    , m_valid{true}
  {
  }

  basic_timestamp_c(T timestamp, bool valid)
    : m_timestamp{timestamp}
    , m_valid{valid}
  {
  }

public:
  basic_timestamp_c()
    : m_timestamp{}
    , m_valid{}
  {
  }

  // validity
  bool valid() const {
    return m_valid;
  }

  void reset() {
    m_valid    = false;
    m_timestamp = std::numeric_limits<T>::min();
  }

  // deconstruction
  T to_ns() const {
    if (!m_valid)
      throw std::domain_error{"invalid timestamp"};
    return m_timestamp;
  }

  T to_ns(T value_if_invalid) const {
    return m_valid ? m_timestamp : value_if_invalid;
  }

  T to_us() const {
    if (!m_valid)
      throw std::domain_error{"invalid timestamp"};
    return m_timestamp / 1000;
  }

  T to_ms() const {
    if (!m_valid)
      throw std::domain_error{"invalid timestamp"};
    return m_timestamp / 1000000;
  }

  T to_s() const {
    if (!m_valid)
      throw std::domain_error{"invalid timestamp"};
    return m_timestamp / 1000000000;
  }

  T to_m() const {
    if (!m_valid)
      throw std::domain_error{"invalid timestamp"};
    return m_timestamp / 60000000000ll;
  }

  T to_h() const {
    if (!m_valid)
      throw std::domain_error{"invalid timestamp"};
    return m_timestamp / 3600000000000ll;
  }

  T to_mpeg() const {
    if (!m_valid)
      throw std::domain_error{"invalid timestamp"};
    return m_timestamp * 9 / 100000;
  }

  T to_samples(T sample_rate) const {
    if (!m_valid)
      throw std::domain_error{"invalid timestamp"};
    if (!sample_rate)
      throw std::domain_error{"invalid sample rate"};
    return (m_timestamp * sample_rate / 100000000 + 5) / 10;
  }

  // arithmetic
  basic_timestamp_c<T> operator +=(basic_timestamp_c<T> const &other) {
    if (!m_valid || !other.m_valid)
      reset();
    else
      m_timestamp += other.m_timestamp;
    return *this;
  }

  basic_timestamp_c<T> operator -=(basic_timestamp_c<T> const &other) {
    if (!m_valid || !other.m_valid)
      reset();
    else
      m_timestamp -= other.m_timestamp;
    return *this;
  }

  basic_timestamp_c<T> operator *=(basic_timestamp_c<T> const &other) {
    if (!m_valid || !other.m_valid)
      reset();
    else
      m_timestamp *= other.m_timestamp;
    return *this;
  }

  basic_timestamp_c<T> operator /=(basic_timestamp_c<T> const &other) {
    if (!m_valid || !other.m_valid)
      reset();
    else
      m_timestamp /= other.m_timestamp;
    return *this;
  }

  basic_timestamp_c<T> abs() const {
    return basic_timestamp_c<T>{std::abs(m_timestamp), m_valid};
  }

  basic_timestamp_c<T> negate() const {
    return basic_timestamp_c<T>{m_timestamp * -1, m_valid};
  }

  basic_timestamp_c<T> value_or_min() const {
    return m_valid ? *this : min();
  }

  basic_timestamp_c<T> value_or_max() const {
    return m_valid ? *this : max();
  }

  basic_timestamp_c<T> value_or_zero() const {
    return m_valid ? *this : ns(0);
  }

  // comparison
  bool operator <(basic_timestamp_c<T> const &other) const {
    return !m_valid &&  other.m_valid ? true
         :  m_valid && !other.m_valid ? false
         :  m_valid &&  other.m_valid ? m_timestamp < other.m_timestamp
         :                              false;
  }

  bool operator ==(basic_timestamp_c<T> const &other) const {
    return (!m_valid && !other.m_valid) || (m_valid && other.m_valid && (m_timestamp == other.m_timestamp));
  }

  // construction
  static basic_timestamp_c<T> ns(T value) {
    return basic_timestamp_c<T>{value};
  }

  static basic_timestamp_c<T> us(T value) {
    return basic_timestamp_c<T>{value * 1000};
  }

  static basic_timestamp_c<T> ms(T value) {
    return basic_timestamp_c<T>{value * 1000000};
  }

  static basic_timestamp_c<T> s(T value) {
    return basic_timestamp_c<T>{value * 1000000000};
  }

  static basic_timestamp_c<T> m(T value) {
    return basic_timestamp_c<T>{value * 60 * 1000000000};
  }

  static basic_timestamp_c<T> h(T value) {
    return basic_timestamp_c<T>{value * 3600 * 1000000000};
  }

  static basic_timestamp_c<T> mpeg(T value) {
    return basic_timestamp_c<T>{value * 100000 / 9};
  }

  static basic_timestamp_c<T> factor(T value) {
    return basic_timestamp_c<T>{value};
  }

  static basic_timestamp_c<T> samples(T samples, T sample_rate) {
    if (!sample_rate)
      throw std::domain_error{"invalid sample rate"};
    return basic_timestamp_c<T>{(samples * 10000000000 / sample_rate + 5) / 10};
  }

  // min/max
  static basic_timestamp_c<T> min() {
    return ns(std::numeric_limits<T>::min());
  }

  static basic_timestamp_c<T> max() {
    return ns(std::numeric_limits<T>::max());
  }
};

using timestamp_c = basic_timestamp_c<int64_t>;
