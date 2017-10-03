/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   definitions for string parsing functions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include <locale.h>

#include <boost/lexical_cast.hpp>

#include "common/math.h"
#include "common/strings/editing.h"
#include "common/timestamp.h"

namespace mtx { namespace conversion {

template <bool is_unsigned>
struct unsigned_checker {
  template<typename StrT>
  static inline void do_check(StrT const &) { }
};

template <>
struct unsigned_checker<true> {
  template<typename StrT>
  static inline void do_check(StrT const &str) {
    if (str[0] == '-')
      boost::throw_exception( boost::bad_lexical_cast{} );
  }
};

}}

bool parse_number_as_rational(std::string const &string, int64_rational_c &value);

template<typename StrT, typename ValueT>
bool
parse_number(StrT const &string,
             ValueT &value) {
  try {
    mtx::conversion::unsigned_checker< std::is_unsigned<ValueT>::value >::do_check(string);
    value = boost::lexical_cast<ValueT>(string);
    return true;
  } catch (boost::bad_lexical_cast &) {
    return false;
  }
}

template<typename StrT>
bool
parse_number(StrT const &string,
             int64_rational_c &value) {
  return parse_number_as_rational(string, value);
}

template<typename StrT>
bool
parse_number(StrT const &string,
             double &value) {
  int64_rational_c rational_value;
  if (!parse_number(string, rational_value))
    return false;

  value = boost::rational_cast<double>(rational_value);

  return true;
}

template<typename StrT>
bool
parse_number(StrT const &string,
             float &value) {
  int64_rational_c rational_value;
  if (!parse_number(string, rational_value))
    return false;

  value = boost::rational_cast<float>(rational_value);

  return true;
}

template <typename StructType, typename DataType>
bool
parse_property_to_struct(std::string const &s,
                         std::map<int64_t, StructType> &property_list) {
  int64_t id = 0;
  auto v     = split(s, ":");

  if ((v.size() != 2) || !parse_number(v[0], id))
    return false;

  v = split(v[1], ",");
  if (v.size() != StructType::num_properties)
    return false;

  std::vector<DataType> data;
  for (int i = 0; i < StructType::num_properties; ++i) {
    DataType value;
    if (!parse_number(v[i], value))
      return false;
    data.push_back(value);
  }

  property_list[id] = StructType(data);

  return true;
}

template <typename DataType>
bool
parse_property_to_value(std::string const &s,
                        std::map<int64_t, DataType> &property_list) {
  int64_t id = 0;
  auto v     = split(s, ":");

  if ((v.size() != 2) || !parse_number(v[0], id))
    return false;

  DataType value;
  if (!parse_number(v[1], value))
    return false;

  property_list[id] = value;

  return true;
}

bool parse_duration_number_with_unit(const std::string &s, int64_t &value);

extern std::string timestamp_parser_error;
extern bool parse_timestamp(const std::string &s, int64_t &timestamp, bool allow_negative = false);
extern bool parse_timestamp(const std::string &s, timestamp_c &timestamp, bool allow_negative = false);

bool parse_bool(std::string value);

uint64_t from_hex(const std::string &data);
