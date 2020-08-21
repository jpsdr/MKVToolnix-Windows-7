/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   locale string splitting and creation

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/locale_string.h"
#include "common/regex.h"

locale_string_c::locale_string_c(std::string locale_string) {
  mtx::regex::jp::Regex locale_re{"^([[:alpha:]]+)?(_[[:alpha:]]+)?(\\.[^@]+)?(@.+)?"};
  mtx::regex::jp::VecNum matches;

  if (!mtx::regex::match(locale_string, matches, locale_re))
    throw mtx::locale_string_format_x(locale_string);

  m_language  = matches[0][1];
  m_territory = matches[0][2];
  m_codeset   = matches[0][3];
  m_modifier  = matches[0][4];

  if (!m_territory.empty())
    m_territory.erase(0, 1);

  if (!m_codeset.empty())
    m_codeset.erase(0, 1);

  if (!m_modifier.empty())
    m_modifier.erase(0, 1);
}

locale_string_c &
locale_string_c::set_codeset_and_modifier(const locale_string_c &src) {
  m_codeset  = src.m_codeset;
  m_modifier = src.m_modifier;

  return *this;
}

std::string
locale_string_c::str(eval_type_e type) {
  std::string locale = m_language;

  if ((type & territory) && !m_territory.empty())
    locale += "_"s + m_territory;

  if ((type & codeset) && !m_codeset.empty())
    locale += "."s + m_codeset;

  if ((type & modifier) && !m_modifier.empty())
    locale += "@"s + m_modifier;

  return locale;
}
