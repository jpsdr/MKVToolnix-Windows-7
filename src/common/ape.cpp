/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   helper functions for APE tags

   Written by James Almer <jamrial@gmail.com>
   Adapted from ID3 parsing code by Moritz Bunkus <moritz@bunkus.org>
*/

#include "common/common_pch.h"

#include "common/at_scope_exit.h"
#include "common/endian.h"
#include "common/mm_io.h"

int
apev2_tag_present_at_end(mm_io_c &io) {
  unsigned char buffer[16];

  io.save_pos();
  at_scope_exit_c restore([&io]() { io.restore_pos(); });

  if (io.setFilePointer2(-32, seek_end) == false)
    return 0;
  if (io.read(buffer, 16) != 16)
    return 0;

  if (   strncmp((char *)buffer, "APETAGEX", 8)
      || get_uint32_le(buffer + 8) > 2000) {
    return 0;
  }

  auto tag_size = get_uint32_le(buffer + 12);
  tag_size     += 32;          // tag footer

  return tag_size;
}

int
ape_tag_present_at_end(mm_io_c &io) {
  return apev2_tag_present_at_end(io);
}
