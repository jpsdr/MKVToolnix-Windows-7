 /*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   IO callback class implementation

   Written by Moritz Bunkus <mo@bunkus.online>.
   Modifications by Peter Niemayer <niemayer@isg.de>.
*/

#include "common/common_pch.h"

#include "common/mm_io_p.h"
#include "common/mm_stdio.h"

/*
   Class for reading from stdin & writing to stdout.
*/

uint64_t
mm_stdio_c::getFilePointer() {
  return 0;
}

void
mm_stdio_c::setFilePointer(int64_t,
                           libebml::seek_mode) {
}

uint32_t
mm_stdio_c::_read(void *buffer,
                  size_t size) {
  return fread(buffer, 1, size, stdin);
}

#if !defined(SYS_WINDOWS)
size_t
mm_stdio_c::_write(const void *buffer,
                   size_t size) {
  p_func()->cached_size = -1;

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
