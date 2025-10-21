/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Definitions for locale handling functions

   Written by Moritz Bunkus <mo@bunkus.online>.
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

inline ::libebml::UTFstring
to_utfstring(std::wstring const &source) {
  return libebml::UTFstring{source};
}

inline ::libebml::UTFstring
to_utfstring(std::string const &source) {
  auto u = libebml::UTFstring{};
  u.SetUTF8(source);
  return u;
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
to_utf8(::libebml::UTFstring const &source) {
  return source.GetUTF8();
}

size_t get_width_in_em(wchar_t c);
size_t get_width_in_em(const std::wstring &s);

namespace mtx::utf8 {

std::string fix_invalid(std::string const &str, uint32_t replacement_marker = 0xfffdu);
bool is_valid(std::string const &str);

}
