/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   definition for the split point class

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

class split_point_c {
public:
  enum type_e {
    duration,
    size,
    timestamp,
    chapter,
    parts,
    parts_frame_field,
    frame_field,
  };

  int64_t m_point;
  type_e m_type;
  bool m_use_once, m_discard, m_create_new_file;

public:
  split_point_c(int64_t point,
                type_e type,
                bool use_once,
                bool discard = false,
                bool create_new_file = true)
    : m_point{point}
    , m_type{type}
    , m_use_once{use_once}
    , m_discard{discard}
    , m_create_new_file{create_new_file}
  {
  }

  bool
  operator <(split_point_c const &rhs)
    const {
    return m_point < rhs.m_point;
  }

  std::string str() const;
};

inline std::ostream &
operator <<(std::ostream &out,
            split_point_c const &sp) {
  out << sp.str();
  return out;
}
