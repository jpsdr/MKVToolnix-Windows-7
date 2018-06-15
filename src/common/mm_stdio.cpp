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

#include "common/mm_stdio.h"

/*
   Class for reading from stdin & writing to stdout.
*/

mm_stdio_c::mm_stdio_c() {
}

uint64
mm_stdio_c::getFilePointer() {
  return 0;
}

void
mm_stdio_c::setFilePointer(int64,
                           libebml::seek_mode) {
}

uint32
mm_stdio_c::_read(void *buffer,
                  size_t size) {
  return fread(buffer, 1, size, stdin);
}

#if !defined(SYS_WINDOWS)
size_t
mm_stdio_c::_write(const void *buffer,
                   size_t size) {
  m_cached_size = -1;

  return fwrite(buffer, 1, size, stdout);
}
#endif // defined(SYS_WINDOWS)

void
mm_stdio_c::close() {
}

void
mm_stdio_c::flush() {
  fflush(stdout);
}
