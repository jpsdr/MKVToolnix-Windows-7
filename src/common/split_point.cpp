/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   the split point class

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/split_point.h"
#include "common/strings/formatting.h"

std::string
split_point_c::str()
  const {
  return fmt::format("<{0} {1} once:{2} discard:{3} create_file:{4}>",
                      mtx::string::format_timestamp(m_point),
                        duration          == m_type ? "duration"
                      : size              == m_type ? "size"
                      : timestamp         == m_type ? "timestamp"
                      : chapter           == m_type ? "chapter"
                      : parts             == m_type ? "part"
                      : parts_frame_field == m_type ? "part(frame/field)"
                      : frame_field       == m_type ? "frame/field"
                      :                               "unknown",
                      m_use_once, m_discard, m_create_new_file);
}
