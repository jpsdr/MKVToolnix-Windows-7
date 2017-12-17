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

#include "common/mm_null_io.h"

/*
   Dummy class for output to /dev/null. Needed for two pass stuff.
*/

mm_null_io_c::mm_null_io_c(std::string const &file_name)
  : m_pos(0)
  , m_file_name(file_name)
{
}

uint64
mm_null_io_c::getFilePointer() {
  return m_pos;
}

void
mm_null_io_c::setFilePointer(int64 offset,
                             seek_mode mode) {
  m_pos = seek_beginning == mode ? offset
        : seek_end       == mode ? 0
        :                          m_pos + offset;
}

uint32
mm_null_io_c::_read(void *buffer,
                    size_t size) {
  memset(buffer, 0, size);
  m_pos += size;

  return size;
}

size_t
mm_null_io_c::_write(const void *,
                     size_t size) {
  m_pos         += size;
  m_cached_size  = -1;

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
  return m_file_name;
}
