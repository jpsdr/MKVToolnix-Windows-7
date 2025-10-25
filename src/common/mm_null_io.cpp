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

#include "common/mm_null_io.h"
#include "common/mm_null_io_p.h"

/*
   Dummy class for output to /dev/null. Needed for two pass stuff.
*/

mm_null_io_c::mm_null_io_c(std::string const &file_name)
  : mm_io_c{*new mm_null_io_private_c{file_name}}
{
}

mm_null_io_c::mm_null_io_c(mm_null_io_private_c &p)
  : mm_io_c{p}
{
}

uint64_t
mm_null_io_c::getFilePointer() {
  return p_func()->pos;
}

void
mm_null_io_c::setFilePointer(int64_t offset,
                             libebml::seek_mode mode) {
  auto p = p_func();

  p->pos = libebml::seek_beginning == mode ? offset
         : libebml::seek_end       == mode ? 0
         :                                   p->pos + offset;
}

uint32_t
mm_null_io_c::_read(void *buffer,
                    size_t size) {
  std::memset(buffer, 0, size);
  p_func()->pos += size;

  return size;
}

size_t
mm_null_io_c::_write(const void *,
                     size_t size) {
  auto p          = p_func();
  p->pos         += size;
  p->cached_size  = -1;

  return size;
}

void
mm_null_io_c::close() {
}

bool
mm_null_io_c::eof() {
  return false;
}

std::string
mm_null_io_c::get_file_name()
  const {
  return p_func()->file_name;
}
