/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   IO callback class definitions

   Written by Moritz Bunkus <mo@bunkus.online>.
   Modifications by Nils Maier <maierman@web.de>
*/

#include "common/common_pch.h"

#include "common/mm_io_x.h"
#include "common/mm_file_io.h"
#include "common/mm_proxy_io.h"
#include "common/mm_write_buffer_io.h"
#include "common/mm_write_buffer_io_p.h"

namespace {
debugging_option_c s_debug_seek{"write_buffer_io|write_buffer_io_seek"}, s_debug_write{"write_buffer_io|write_buffer_io_write"};
}

mm_write_buffer_io_c::mm_write_buffer_io_c(mm_io_cptr const &out,
                                           std::size_t buffer_size)
  : mm_proxy_io_c{*new mm_write_buffer_io_private_c{out, buffer_size}}
{
}

mm_write_buffer_io_c::mm_write_buffer_io_c(mm_write_buffer_io_private_c &p)
  : mm_proxy_io_c{p}
{
}

mm_write_buffer_io_c::~mm_write_buffer_io_c() {
  close_write_buffer_io();
}

mm_io_cptr
mm_write_buffer_io_c::open(const std::string &file_name,
                           size_t buffer_size) {
  return std::make_shared<mm_write_buffer_io_c>(std::make_shared<mm_file_io_c>(file_name, libebml::MODE_CREATE), buffer_size);
}

uint64_t
mm_write_buffer_io_c::getFilePointer() {
  return mm_proxy_io_c::getFilePointer() + p_func()->fill;
}

void
mm_write_buffer_io_c::setFilePointer(int64_t offset,
                                     libebml::seek_mode mode) {
  int64_t new_pos
    = libebml::seek_beginning == mode ? offset
    : libebml::seek_end       == mode ? p_func()->proxy_io->get_size() + offset // offsets from the end are negative already
    :                                   getFilePointer()               + offset;

  if (new_pos == static_cast<int64_t>(getFilePointer()))
    return;

  flush_buffer();

  if (s_debug_seek) {
    int64_t previous_pos = mm_proxy_io_c::getFilePointer();
    mxdebug(fmt::format("seek from {0} to {1} diff {2}\n", previous_pos, new_pos, new_pos - previous_pos));
  }

  mm_proxy_io_c::setFilePointer(offset, mode);
}

void
mm_write_buffer_io_c::flush() {
  flush_buffer();
  mm_proxy_io_c::flush();
}

void
mm_write_buffer_io_c::close() {
  close_write_buffer_io();
}

void
mm_write_buffer_io_c::close_write_buffer_io() {
  flush_buffer();
  mm_proxy_io_c::close();
}

uint32_t
mm_write_buffer_io_c::_read(void *buffer,
                            size_t size) {
  flush_buffer();
  return mm_proxy_io_c::_read(buffer, size);
}

size_t
mm_write_buffer_io_c::_write(const void *buffer,
                             size_t size) {
  auto p = p_func();

  size_t avail;
  const char *buf = static_cast<const char *>(buffer);
  size_t remain   = size;

  // whole blocks
  while (remain >= (avail = p->size - p->fill)) {
    if (p->fill) {
      // Fill the buffer in an attempt to defeat potentially
      // lousy OS I/O scheduling
      memcpy(p->buffer + p->fill, buf, avail);
      p->fill = p->size;
      flush_buffer();
      remain -= avail;
      buf    += avail;

    } else {
      // write whole blocks, skipping the buffer
      avail = mm_proxy_io_c::_write(buf, p->size);
      if (avail != p->size)
        throw mtx::mm_io::insufficient_space_x();

      remain -= avail;
      buf    += avail;
    }
  }

  if (remain) {
    memcpy(p->buffer + p->fill, buf, remain);
    p->fill += remain;
  }

  p->cached_size = -1;

  return size;
}

void
mm_write_buffer_io_c::flush_buffer() {
  auto p = p_func();

  if (!p->fill)
    return;

  size_t written = mm_proxy_io_c::_write(p->buffer, p->fill);
  size_t fill    = p->fill;
  p->fill         = 0;

  mxdebug_if(s_debug_write, fmt::format("flush_buffer() at {0} for {1} written {2}\n", mm_proxy_io_c::getFilePointer() - written, fill, written));

  if (written != fill)
    throw mtx::mm_io::insufficient_space_x();
}

void
mm_write_buffer_io_c::discard_buffer() {
  p_func()->fill = 0;
}
