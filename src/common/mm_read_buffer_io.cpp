/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   IO callback class definitions

   Written by Moritz Bunkus <mo@bunkus.online>.
   Rewritten by Nils Maier <maierman@web.de>
*/

#include "common/common_pch.h"

#include "common/mm_io_x.h"
#include "common/mm_proxy_io.h"
#include "common/mm_read_buffer_io.h"
#include "common/mm_read_buffer_io_p.h"

namespace {
debugging_option_c s_debug_seek{"read_buffer_io|read_buffer_io_seek"}, s_debug_read{"read_buffer_io|read_buffer_io_read"};
}

mm_read_buffer_io_c::mm_read_buffer_io_c(mm_io_cptr const &in,
                                         std::size_t buffer_size)
  : mm_proxy_io_c{*new mm_read_buffer_io_private_c{in, buffer_size}}
{
}

mm_read_buffer_io_c::mm_read_buffer_io_c(mm_read_buffer_io_private_c &p)
  : mm_proxy_io_c{p}
{
}

mm_read_buffer_io_c::~mm_read_buffer_io_c() {
  close();
}

uint64_t
mm_read_buffer_io_c::getFilePointer() {
  auto p = p_func();

  return p->buffering ? p->offset + p->cursor : p->proxy_io->getFilePointer();
}

void
mm_read_buffer_io_c::setFilePointer(int64_t offset,
                                    libebml::seek_mode mode) {
  auto p = p_func();

  if (!p->buffering) {
    p->proxy_io->setFilePointer(offset, mode);
    return;
  }

  int64_t new_pos = 0;
  // FIXME int64_t overflow

  // No need to actually compute this here; _read() will do just that
  p->eof = false;

  switch (mode) {
    case libebml::seek_beginning:
      new_pos = offset;
      break;

    case libebml::seek_current:
      new_pos  = p->offset;
      new_pos += p->cursor;
      new_pos += offset;
      break;

    case libebml::seek_end:
      new_pos = static_cast<int64_t>(get_size()) + offset; // offsets from the end are negative already
      break;

    default:
      throw mtx::mm_io::seek_x();
  }

  // Still within the current buffer?
  int64_t in_buf = new_pos - p->offset;
  if ((0 <= in_buf) && (in_buf <= static_cast<int64_t>(p->fill))) {
    p->cursor = in_buf;
    return;
  }

  int64_t previous_pos = p->proxy_io->getFilePointer();

  // Actual seeking
  p->proxy_io->setFilePointer(std::min(new_pos, get_size()));

  // Get the actual offset from the underlying stream
  // Better be safe than sorry and use this instead of just taking
  p->offset = p->proxy_io->getFilePointer();

  // "Drop" the buffer content
  p->cursor = p->fill = 0;

  mxdebug_if(s_debug_seek, fmt::format("seek on proxy from {0} to {1} relative {2}\n", previous_pos, p->offset, p->offset - previous_pos));
}

int64_t
mm_read_buffer_io_c::get_size() {
  return p_func()->proxy_io->get_size();
}

uint32_t
mm_read_buffer_io_c::_read(void *buffer,
                           size_t size) {
  auto p = p_func();

  if (!p->buffering)
    return p->proxy_io->read(buffer, size);

  char *buf    = static_cast<char *>(buffer);
  uint32_t res = 0;

  while (0 < size) {
    // TODO Directly write full blocks into the output buffer when size > p->size
    size_t avail = std::min(size, p->fill - p->cursor);
    if (avail) {
      memcpy(buf, p->buffer + p->cursor, avail);
      buf      += avail;
      res      += avail;
      size     -= avail;
      p->cursor += avail;

    } else {
      // Refill the buffer
      p->offset += p->cursor;
      p->cursor  = 0;
      p->fill    = 0;
      avail     = std::min(get_size() - p->offset, static_cast<int64_t>(p->af_buffer->get_size()));

      if (!avail) {
        // must keep track of eof, as p->proxy_io->eof() will never be reached
        // because of the above eof calculation
        p->eof = true;
        break;
      }

      int64_t previous_pos = p->proxy_io->getFilePointer();

      p->fill = p->proxy_io->read(p->buffer, avail);
      mxdebug_if(s_debug_read, fmt::format("physical read from position {2} for {0} returned {1}\n", avail, p->fill, previous_pos));
      if (p->fill != avail) {
        p->eof = true;
        if (!p->fill)
          break;
      }
    }
  }

  return res;
}

size_t
mm_read_buffer_io_c::_write(const void *,
                            size_t) {
  throw mtx::mm_io::wrong_read_write_access_x();
  return 0;
}

void
mm_read_buffer_io_c::enable_buffering(bool enable) {
  auto p = p_func();

  p->buffering = enable;
  if (!p->buffering) {
    p->offset = 0;
    p->cursor = 0;
    p->fill   = 0;
  }
}

void
mm_read_buffer_io_c::set_buffer_size(std::size_t new_buffer_size) {
  auto p = p_func();

  if (new_buffer_size == p->af_buffer->get_size())
    return;

  p->af_buffer->resize(new_buffer_size);
  p->buffer = p->af_buffer->get_buffer();

  if (!p->buffering)
    return;

  auto previous_pos = getFilePointer();
  p->offset         = previous_pos;
  p->cursor         = 0;
  p->fill           = 0;

  p->proxy_io->setFilePointer(previous_pos);
}

bool
mm_read_buffer_io_c::eof() {
  return p_func()->eof;
}

void
mm_read_buffer_io_c::clear_eof() {
  p_func()->eof = false;
}
