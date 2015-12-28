/** MPEG helper functions

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   \author Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/mpeg.h"

namespace mpeg {

memory_cptr
nalu_to_rbsp(memory_cptr const &buffer) {
  int pos, size = buffer->get_size();
  mm_mem_io_c d(nullptr, size, 100);
  unsigned char *b = buffer->get_buffer();

  for (pos = 0; pos < size; ++pos) {
    if (   ((pos + 2) < size)
        && (0 == b[pos])
        && (0 == b[pos + 1])
        && (3 == b[pos + 2])) {
      d.write_uint8(0);
      d.write_uint8(0);
      pos += 2;

    } else
      d.write_uint8(b[pos]);
  }

  return memory_c::clone(d.get_and_lock_buffer(), d.getFilePointer());
}

memory_cptr
rbsp_to_nalu(memory_cptr const &buffer) {
  int pos, size = buffer->get_size();
  mm_mem_io_c d(nullptr, size, 100);
  unsigned char *b = buffer->get_buffer();

  for (pos = 0; pos < size; ++pos) {
    if (   ((pos + 2) < size)
        && (0 == b[pos])
        && (0 == b[pos + 1])
        && (3 >= b[pos + 2])) {
      d.write_uint8(0);
      d.write_uint8(0);
      d.write_uint8(3);
      ++pos;

    } else
      d.write_uint8(b[pos]);
  }

  return memory_c::clone(d.get_and_lock_buffer(), d.getFilePointer());
}

}
