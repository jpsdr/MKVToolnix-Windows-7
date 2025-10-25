/** MPEG helper functions

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   \author Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/debugging.h"
#include "common/endian.h"
#include "common/mm_mem_io.h"
#include "common/mpeg.h"

namespace mtx::mpeg {

memory_cptr
nalu_to_rbsp(memory_cptr const &buffer) {
  mm_mem_io_cptr d;
  auto b = buffer->get_buffer();

  for (int pos = 0, size = buffer->get_size(); pos < size; ++pos) {
    if (   ((pos + 2) < size)
        && (0 == b[pos])
        && (0 == b[pos + 1])
        && (3 == b[pos + 2])) {

      if (!d) {
        d = std::make_shared<mm_mem_io_c>(nullptr, size, 100);
        if (pos > 0)
          d->write(b, pos);
      }

      d->write_uint8(0);
      d->write_uint8(0);
      pos += 2;

    } else if (d)
      d->write_uint8(b[pos]);
  }

  if (!d)
    return buffer;

  return d->get_and_lock_buffer();
}

memory_cptr
rbsp_to_nalu(memory_cptr const &buffer) {
  int pos, size = buffer->get_size();
  mm_mem_io_c d(nullptr, size, 100);
  uint8_t *b = buffer->get_buffer();

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

  return d.get_and_lock_buffer();
}

void
write_nalu_size(uint8_t *buffer,
                std::size_t size,
                std::size_t nalu_size_length) {
  put_uint_be(buffer, size, nalu_size_length);
}

memory_cptr
create_nalu_with_size(memory_cptr const &src,
                      std::size_t nalu_size_length,
                      std::vector<memory_cptr> extra_data) {
  auto final_size = nalu_size_length + src->get_size();

  for (auto &mem : extra_data)
    final_size += mem->get_size();

  auto buffer = memory_c::alloc(final_size);
  auto dest   = buffer->get_buffer();

  for (auto &mem : extra_data) {
    memcpy(dest, mem->get_buffer(), mem->get_size());
    dest += mem->get_size();
  }

  auto nalu_size = src->get_size();
  write_nalu_size(dest, nalu_size, nalu_size_length);
  memcpy(dest + nalu_size_length, src->get_buffer(), nalu_size);

  return buffer;
}

void
remove_trailing_zero_bytes(memory_c &buffer) {
  static debugging_option_c s_debug_trailing_zero_byte_removal{"avc_parser|avc_trailing_zero_byte_removal"};

  auto size = buffer.get_size();

  if (!size)
    return;

  auto bytes = buffer.get_buffer();
  auto idx   = 0u;

  while ((idx < size) && !bytes[size - idx - 1])
    ++idx;

  auto new_size = size - idx;
  buffer.set_size(new_size);

  mxdebug_if(s_debug_trailing_zero_byte_removal, fmt::format("Removing trailing zero bytes from old size {0} down to new size {1}, removed {2}\n", size, new_size, idx));
}

}
