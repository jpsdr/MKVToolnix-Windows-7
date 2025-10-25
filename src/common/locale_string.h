/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   locale string splitting and creation

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/error.h"

namespace mtx {
  class locale_string_format_x: public exception {
  protected:
    std::string m_format;
  public:
    locale_string_format_x(const std::string &format) : m_format{format} { }
    virtual ~locale_string_format_x() throw() { }

    virtual const char *what() const throw() {
      return m_format.c_str();
    }
  };
}

class locale_string_c {
protected:
  std::string m_language, m_territory, m_codeset, m_modifier;

public:
  enum eval_type_e {
    language  = 0,
    territory = 1,
    codeset   = 2,
    modifier  = 4,

    half      = 1,
    full      = 7,
  };

public:
  locale_string_c(std::string locale);

  locale_string_c &set_codeset_and_modifier(const locale_string_c &src);
  std::string str(eval_type_e type = full);
};
