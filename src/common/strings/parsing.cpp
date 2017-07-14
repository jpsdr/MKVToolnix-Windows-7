/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   string parsing helper functions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/list_utils.h"
#include "common/math.h"
#include "common/strings/formatting.h"
#include "common/strings/parsing.h"

std::string timestamp_parser_error;

inline bool
set_tcp_error(const std::string &error) {
  timestamp_parser_error = error;
  return false;
}

inline bool
set_tcp_error(const boost::format &error) {
  timestamp_parser_error = error.str();
  return false;
}

bool
parse_number_as_rational(std::string const &string,
                         int64_rational_c &value) {
  auto parts = split(string, ".", 2);

  while (parts.size() < 2)
    parts.emplace_back("");

  int64_t num{0};
  if (!parts[0].length() || !parse_number(parts[0], num))
    return false;

  auto p1        = int64_rational_c{num, 1},
    p2           = int64_rational_c{0, 1};
  auto const len = parts[1].length();

  if (len > 0) {
    num = 0;
    if (!parse_number(parts[1], num))
      return false;

    auto den = 1ll;
    for (auto idx = 0u; idx < len; ++idx)
      den *= 10;

    p2 = int64_rational_c{num, den};
  }

  value = p1 + p2;

  return true;
}

bool
parse_timestamp(const std::string &src,
                int64_t &timestamp,
                bool allow_negative) {
  // Recognized format:
  // 1. XXXXXXXu   with XXXXXX being a number followed
  //    by one of the units 's', 'ms', 'us' or 'ns'
  // 2. HH:MM:SS.nnnnnnnnn  with up to nine digits 'n' for ns precision;
  //    HH: is optional; HH, MM and SS can be either one or two digits.
  int h, m, s, n, values[4], num_values, num_digits, num_colons, negative = 1;
  size_t offset = 0, i;
  bool decimal_point_found;

  if (src.empty())
    return set_tcp_error(Y("Invalid format: the string is empty."));

  if ('-' == src[0]) {
    if (!allow_negative)
      return false;
    negative = -1;
    offset = 1;
  }

  try {
    if (src.length() < (2 + offset))
      throw false;

    std::string unit = src.substr(src.length() - 2, 2);

    int64_t value = 0;
    if (!parse_duration_number_with_unit(src.substr(offset, src.length() - offset), value))
      throw false;

    timestamp = value * negative;

    return true;
  } catch (...) {
  }

  num_digits = 0;
  num_colons = 0;
  num_values = 1;
  decimal_point_found = false;
  memset(&values, 0, sizeof(int) * 4);

  for (i = offset; src.length() > i; ++i) {
    if (isdigit(src[i])) {
      if (decimal_point_found && (9 == num_digits))
        return set_tcp_error(Y("Invalid format: More than nine nano-second digits"));
      values[num_values - 1] = values[num_values - 1] * 10 + src[i] - '0';
      ++num_digits;

    } else if (('.' == src[i]) ||
               ((':' == src[i]) && (2 == num_colons))) {
      if (decimal_point_found)
        return set_tcp_error(Y("Invalid format: Second decimal point after first decimal point"));

      if (0 == num_digits)
        return set_tcp_error(Y("Invalid format: No digits before decimal point"));
      ++num_values;
      num_digits = 0;
      decimal_point_found = true;

    } else if (':' == src[i]) {
      if (decimal_point_found)
        return set_tcp_error(Y("Invalid format: Colon inside nano-second part"));
      if (2 == num_colons)
        return set_tcp_error(Y("Invalid format: More than two colons"));
      if (0 == num_digits)
        return set_tcp_error(Y("Invalid format: No digits before colon"));
      ++num_colons;
      ++num_values;
      num_digits = 0;

    } else
      return set_tcp_error(boost::format(Y("Invalid format: unknown character '%1%' found")) % src[i]);
  }

  if (1 > num_colons)
    return set_tcp_error(Y("Invalid format: At least minutes and seconds have to be given, but no colon was found"));

  if ((':' == src[src.length() - 1]) || ('.' == src[src.length() - 1]))
    return set_tcp_error(Y("Invalid format: The last character is a colon or a decimal point instead of a digit"));

  // No error has been found. Now find out whoich parts have been
  // set and which haven't.

  if (4 == num_values) {
    h = values[0];
    m = values[1];
    s = values[2];
    n = values[3];
    for (; 9 > num_digits; ++num_digits)
      n *= 10;

  } else if (2 == num_values) {
    h = 0;
    m = values[0];
    s = values[1];
    n = 0;

  } else if (decimal_point_found) {
    h = 0;
    m = values[0];
    s = values[1];
    n = values[2];
    for (; 9 > num_digits; ++num_digits)
      n *= 10;

  } else {
    h = values[0];
    m = values[1];
    s = values[2];
    n = 0;

  }

  if (m > 59)
    return set_tcp_error(boost::format(Y("Invalid number of minutes: %1% > 59")) % m);
  if (s > 59)
    return set_tcp_error(boost::format(Y("Invalid number of seconds: %1% > 59")) % s);

  timestamp              = (((int64_t)h * 60 * 60 + (int64_t)m * 60 + (int64_t)s) * 1000000000ll + n) * negative;
  timestamp_parser_error = Y("no error");

  return true;
}

bool
parse_timestamp(const std::string &src,
                timestamp_c &timestamp,
                bool allow_negative) {
  int64_t tmp{};
  if (!parse_timestamp(src, tmp, allow_negative))
    return false;

  timestamp = timestamp_c::ns(tmp);

  return true;
}

/** \brief Parse a string for a boolean value

   Interpretes the string \c orig as a boolean value. Accepted
   is "true", "yes", "1" as boolean true and "false", "no", "0"
   as boolean false.
*/
bool
parse_bool(std::string value) {
  balg::to_lower(value);

  if ((value == "yes") || (value == "true") || (value == "1"))
    return true;
  if ((value == "no") || (value == "false") || (value == "0"))
    return false;
  throw false;
}

/** \brief Parse a number postfixed with a time-based unit

   This function parsers a number that is postfixed with one of the
   units 's', 'ms', 'us', 'ns', 'fps', 'p' or 'i'. If the postfix is
   'fps' then this means 'frames per second'. If the postfix is 'p' or
   'i' then it is also interpreted as the number of 'progressive
   frames per second' or 'interlaced frames per second'.

   It returns a number of nanoseconds.
*/
bool
parse_duration_number_with_unit(const std::string &s,
                                int64_t &value) {
  boost::regex re1("(-?\\d+\\.?\\d*)(h|m|min|s|ms|msec|us|µs|ns|nsec|fps|p|i)?",  boost::regex::perl | boost::regex::icase);
  boost::regex re2("(-?\\d+)/(-?\\d+)(h|m|min|s|ms|msec|us|µs|ns|nsec|fps|p|i)?", boost::regex::perl | boost::regex::icase);

  std::string unit, s_n, s_d;
  int64_rational_c r{0, 1};

  boost::smatch matches;
  if (boost::regex_match(s, matches, re1)) {
    if (!parse_number(matches[1], r))
      return false;

    if (matches.size() > 2)
      unit = matches[2];

  } else if (boost::regex_match(s, matches, re2)) {
    int64_t n, d;
    if (!parse_number(matches[1], n) || !parse_number(matches[2], d))
      return false;

    r = int64_rational_c{n, d};

    if (matches.size() > 3)
      unit = matches[3];

  } else
    return false;

  balg::to_lower(unit);

  if ((unit == "fps") || (unit == "p") || (unit == "i")) {
    if (unit == "i")
      r *= int64_rational_c{1, 2};

    if (int64_rational_c{2396, 100} == r)
      r = int64_rational_c{24000, 1001};

    else if (int64_rational_c{29976, 1000} == r)
      r = int64_rational_c{30000, 1001};

    else if (int64_rational_c{5994, 100} == r)
      r = int64_rational_c{60000, 1001};

    value = boost::rational_cast<int64_t>(int64_rational_c{1000000000ll, 1} / r);

    return true;
  }

  int64_t multiplier = 1000000000; // default: 1s

  if (mtx::included_in(unit, "h"))
    multiplier = 3600ll * 1000000000;
  else if (mtx::included_in(unit, "m", "min"))
    multiplier = 60ll * 1000000000;
  else if (mtx::included_in(unit, "ms", "msec"))
    multiplier = 1000000;
  else if (mtx::included_in(unit, "us", "µs"))
    multiplier = 1000;
  else if (mtx::included_in(unit, "ns", "nsec"))
    multiplier = 1;
  else if (unit != "s")
    return false;

  value = boost::rational_cast<int64_t>(r * int64_rational_c{multiplier, 1});

  return true;
}

uint64_t
from_hex(const std::string &data) {
  const char *s = data.c_str();
  if (*s == 0)
    throw std::bad_cast();

  uint64_t value = 0;

  while (*s) {
    unsigned int digit = isdigit(*s)                  ? *s - '0'
                       : (('a' <= *s) && ('f' >= *s)) ? *s - 'a' + 10
                       : (('A' <= *s) && ('F' >= *s)) ? *s - 'A' + 10
                       :                                16;
    if (16 == digit)
      throw std::bad_cast();

    value = (value << 4) + digit;
    ++s;
  }

  return value;
}
