 /*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   IO callback class implementation

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>

#include "common/mm_io_x.h"
#include "common/mm_file_io.h"

mm_file_io_c::mm_file_io_c(const std::string &path,
                           const open_mode mode)
  : m_file_name(path)
  , m_file(nullptr)
{
  const char *cmode;

  switch (mode) {
    case MODE_READ:
      cmode = "rb";
      break;
    case MODE_WRITE:
      cmode = "r+b";
      break;
    case MODE_CREATE:
      cmode = "w+b";
      break;
    case MODE_SAFE:
      cmode = "rb";
      break;
    default:
      throw mtx::invalid_parameter_x();
  }

  if ((MODE_WRITE == mode) || (MODE_CREATE == mode))
    prepare_path(path);
  std::string local_path = g_cc_local_utf8->native(path);

  struct stat st;
  if ((0 == stat(local_path.c_str(), &st)) && S_ISDIR(st.st_mode))
    throw mtx::mm_io::open_x{mtx::mm_io::make_error_code()};

  m_file = (FILE *)fopen(local_path.c_str(), cmode);

  if (!m_file)
    throw mtx::mm_io::open_x{mtx::mm_io::make_error_code()};
}

void
mm_file_io_c::setFilePointer(int64 offset,
                             libebml::seek_mode mode) {
  int whence = mode == libebml::seek_beginning ? SEEK_SET
             : mode == libebml::seek_end       ? SEEK_END
             :                                   SEEK_CUR;

  if (fseeko((FILE *)m_file, offset, whence) != 0)
    throw mtx::mm_io::seek_x{mtx::mm_io::make_error_code()};

  m_current_position = ftello((FILE *)m_file);
}

size_t
mm_file_io_c::_write(const void *buffer,
                     size_t size) {
  size_t bwritten = fwrite(buffer, 1, size, (FILE *)m_file);
  if (ferror((FILE *)m_file) != 0)
    throw mtx::mm_io::read_write_x{mtx::mm_io::make_error_code()};

  m_current_position += bwritten;
  m_cached_size       = -1;

  return bwritten;
}

uint32
mm_file_io_c::_read(void *buffer,
                    size_t size) {
  int64_t bread = fread(buffer, 1, size, (FILE *)m_file);

  m_current_position += bread;

  return bread;
}

void
mm_file_io_c::close() {
  if (m_file) {
    fclose((FILE *)m_file);
    m_file = nullptr;
  }
}

bool
mm_file_io_c::eof() {
  return feof((FILE *)m_file) != 0;
}

void
mm_file_io_c::clear_eof() {
  clearerr(static_cast<FILE *>(m_file));
}

int
mm_file_io_c::truncate(int64_t pos) {
  m_cached_size = -1;
  return ftruncate(fileno((FILE *)m_file), pos);
}

/** \brief OS and kernel dependant setup
*/
void
mm_file_io_c::setup() {
}
