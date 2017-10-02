/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Definitions for locale handling functions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include <ebml/EbmlUnicodeString.h>

std::wstring to_wide(const std::string &source);

inline std::wstring
to_wide(wchar_t const *source) {
  return source;
}

inline std::wstring
to_wide(std::wstring const &source) {
  return source;
}

inline std::wstring
to_wide(boost::wformat const &source) {
  return source.str();
}

inline std::wstring
to_wide(::libebml::UTFstring const &source) {
  return source.c_str();
}

inline ::libebml::UTFstring
to_utfstring(std::wstring const &source) {
  return UTFstring{source};
}

inline ::libebml::UTFstring
to_utfstring(boost::wformat const &source) {
  return to_utfstring(source.str());
}

inline ::libebml::UTFstring
to_utfstring(std::string const &source) {
  auto u = UTFstring{};
  u.SetUTF8(source);
  return u;
}

inline ::libebml::UTFstring
to_utfstring(boost::format const &source) {
  return to_utfstring(source.str());
}

std::string to_utf8(const std::wstring &source);

inline std::string
to_utf8(char const *source) {
  return source;
}

inline std::string
to_utf8(std::string const &source) {
  return source;
}

inline std::string
to_utf8(boost::format const &source) {
  return source.str();
}

inline std::string
to_utf8(::libebml::UTFstring const &source) {
  return source.GetUTF8();
}

size_t get_width_in_em(wchar_t c);
size_t get_width_in_em(const std::wstring &s);
