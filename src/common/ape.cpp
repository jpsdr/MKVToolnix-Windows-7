/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   helper functions for APE tags

   Written by James Almer <jamrial@gmail.com>
   Adapted from ID3 parsing code by Moritz Bunkus <mo@bunkus.online>
*/

#include "common/common_pch.h"

#include "common/at_scope_exit.h"
#include "common/endian.h"
#include "common/mm_io.h"

namespace mtx::ape {

int
tag_present_at_end(mm_io_c &io) {
  uint8_t buffer[24];

  io.save_pos();
  at_scope_exit_c restore([&io]() { io.restore_pos(); });

  if (io.setFilePointer2(-32, libebml::seek_end) == false)
    return 0;
  if (io.read(buffer, 24) != 24)
    return 0;

  if (   strncmp((char *)buffer, "APETAGEX", 8)
      || get_uint32_le(buffer + 8) > 2000) {
    return 0;
  }

  auto tag_size = get_uint32_le(buffer + 12);
  auto flags    = get_uint32_le(buffer + 20);
  if (flags & (1U << 31))
      tag_size += 32;          // tag header

  return tag_size;
}

}
