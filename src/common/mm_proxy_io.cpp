/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   IO callback class implementation

   Written by Moritz Bunkus <moritz@bunkus.org>.
   Modifications by Peter Niemayer <niemayer@isg.de>.
*/

#include "common/common_pch.h"

#include "common/mm_proxy_io.h"

/*
   Proxy class that does I/O on a mm_io_c handed over in the ctor.
   Useful for e.g. doing text I/O on other I/Os (file, mem).
*/

void
mm_proxy_io_c::close() {
  if (m_proxy_delete_io)
    delete m_proxy_io;

  m_proxy_io = nullptr;
}


uint32
mm_proxy_io_c::_read(void *buffer,
                     size_t size) {
  return m_proxy_io->read(buffer, size);
}

size_t
mm_proxy_io_c::_write(const void *buffer,
                      size_t size) {
  m_cached_size = -1;
  return m_proxy_io->write(buffer, size);
}
