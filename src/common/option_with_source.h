/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Option with a priority/source

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

enum class option_source_e {
    none         =  0
  , bitstream    = 10
  , container    = 20
  , command_line = 30
};

template<typename T>
class option_with_source_c {
public:
protected:
  option_source_e m_source;
  std::optional<T> m_value;

public:
  option_with_source_c()
    : m_source{option_source_e::none}
  {
  }

  option_with_source_c(T const &value,
                       option_source_e source)
    : m_source{option_source_e::none}
  {
    set(value, source);
  }

  operator bool()
    const {
    return m_value.has_value();
  }

  bool
  has_value()
    const {
    return m_value.has_value();
  }

  T const &
  get()
    const {
    if (!*this)
      throw std::logic_error{"not set yet"};
    return m_value.value();
  }

  T const &
  value()
    const {
    return get();
  }

  option_source_e
  get_source()
    const {
    if (!*this)
      throw std::logic_error{"not set yet"};
    return m_source;
  }

  void
  set(T const &value,
      option_source_e source) {
    if (source < m_source)
      return;

    m_value  = value;
    m_source = source;
  }
};
