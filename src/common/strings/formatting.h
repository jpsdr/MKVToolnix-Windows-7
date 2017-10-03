/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   string formatting functions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include <ostream>
#include <sstream>

#include "common/strings/editing.h"
#include "common/timestamp.h"

#define WRAP_AT_TERMINAL_WIDTH -1

std::string format_timestamp(int64_t timestamp, unsigned int precision = 9);
std::string format_timestamp(int64_t timestamp, std::string const &format);

template<typename T>
std::string
format_timestamp(basic_timestamp_c<T> const &timestamp,
                 unsigned int precision = 9) {
  if (!timestamp.valid())
    return "<InvTC>";
  return format_timestamp(timestamp.to_ns(), precision);
}

template<typename T>
std::string
format_timestamp(basic_timestamp_c<T> const &timestamp,
                 std::string const &format) {
  if (!timestamp.valid())
    return "<InvTC>";
  return format_timestamp(timestamp.to_ns(), format);
}

template<typename T>
std::ostream &
operator <<(std::ostream &out,
            basic_timestamp_c<T> const &timestamp) {
  if (timestamp.valid())
    out << format_timestamp(timestamp);
  else
    out << "<InvTC>";
  return out;
}

std::string format_file_size(int64_t size);

std::string format_paragraph(const std::string &text_to_wrap,
                             int indent_column                    = 0,
                             const std::string &indent_first_line = empty_string,
                             std::string indent_following_lines   = empty_string,
                             int wrap_column                      = WRAP_AT_TERMINAL_WIDTH,
                             const char *break_chars              = " ,.)/:");

std::wstring format_paragraph(const std::wstring &text_to_wrap,
                              int indent_column                     = 0,
                              const std::wstring &indent_first_line = L" ",
                              std::wstring indent_following_lines   = L" ",
                              int wrap_column                       = WRAP_AT_TERMINAL_WIDTH,
                              const std::wstring &break_chars       = L" ,.)/:");

void fix_format(const char *fmt, std::string &new_fmt);

inline std::string
to_string(std::string const &value) {
  return value;
}

std::string to_string(double value, unsigned int precision);
std::string to_string(int64_t numerator, int64_t denominator, unsigned int precision);
std::string format_number(int64_t number);
std::string format_number(uint64_t number);

template<typename T>
std::string
to_string(basic_timestamp_c<T> const &timestamp) {
  return format_timestamp(timestamp.to_ns());
}


template<typename T>
std::string
to_string(T const &value) {
  std::stringstream ss;
  ss << value;
  return ss.str();
}

std::string to_hex(const unsigned char *buf, size_t size, bool compact = false);
inline std::string
to_hex(const std::string &buf,
       bool compact = false) {
  return to_hex(reinterpret_cast<const unsigned char *>(buf.c_str()), buf.length(), compact);
}
inline std::string
to_hex(memory_cptr const &buf,
       bool compact = false) {
  return to_hex(buf->get_buffer(), buf->get_size(), compact);
}
inline std::string
to_hex(EbmlBinary *bin,
       bool compact = false) {
  return to_hex(bin->GetBuffer(), bin->GetSize(), compact);
}
inline std::string
to_hex(EbmlBinary &bin,
       bool compact = false) {
  return to_hex(bin.GetBuffer(), bin.GetSize(), compact);
}

std::string create_minutes_seconds_time_string(unsigned int seconds, bool omit_minutes_if_zero = false);
