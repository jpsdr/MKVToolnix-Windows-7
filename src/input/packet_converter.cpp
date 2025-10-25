/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   base packet converter

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "input/packet_converter.h"

bool
packet_converter_c::convert_for_pid(packet_cptr const &packet,
                                    uint16_t pid) {
  m_current_pid = pid;
  return convert(packet);
}
