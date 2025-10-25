 /*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   IO callback class implementation

   Written by Moritz Bunkus <mo@bunkus.online>.
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
#include "common/mm_file_io_p.h"
#if defined(SYS_APPLE)
# include "common/fs_sys_helpers.h"
#endif

mm_file_io_private_c::mm_file_io_private_c(std::string const &p_file_name,
                                           libebml::open_mode const p_mode)
  : file_name{p_file_name}
  , mode{p_mode}
{
#if defined(SYS_APPLE)
  file_name = mtx::sys::normalize_unicode_string(file_name, mtx::sys::unicode_normalization_form_e::d);
#endif  // SYS_APPLE

  const char *cmode;

  switch (mode) {
    case libebml::MODE_READ:
      cmode = "rb";
      break;
    case libebml::MODE_WRITE:
      cmode = "r+b";
      break;
    case libebml::MODE_CREATE:
      cmode = "w+b";
      break;
    case libebml::MODE_SAFE:
      cmode = "rb";
      break;
    default:
      throw mtx::invalid_parameter_x();
  }

  if ((libebml::MODE_WRITE == mode) || (libebml::MODE_CREATE == mode))
    mm_file_io_c::prepare_path(file_name);
  std::string local_path = g_cc_local_utf8->native(file_name);

  struct stat st;
  if ((0 == stat(local_path.c_str(), &st)) && S_ISDIR(st.st_mode))
    throw mtx::mm_io::open_x{mtx::mm_io::make_error_code()};

  file = fopen(local_path.c_str(), cmode);

#if defined(SYS_APPLE)
  if (!file && (libebml::MODE_CREATE != mode)) {
    // When reading files on macOS retry with names in NFC as they
    // might come from other sources, e.g. via NFS mounts from NFC
    // systems such as Linux.
    local_path = g_cc_local_utf8->native(mtx::sys::normalize_unicode_string(file_name, mtx::sys::unicode_normalization_form_e::c));
    file = fopen(local_path.c_str(), cmode);
  }
#endif  // SYS_APPLE

  if (!file)
    throw mtx::mm_io::open_x{mtx::mm_io::make_error_code()};
}

void
mm_file_io_c::setFilePointer(int64_t offset,
                             libebml::seek_mode mode) {
  auto p     = p_func();
  int whence = mode == libebml::seek_beginning ? SEEK_SET
             : mode == libebml::seek_end       ? SEEK_END
             :                                   SEEK_CUR;

  if (fseeko(p->file, offset, whence) != 0)
    throw mtx::mm_io::seek_x{mtx::mm_io::make_error_code()};

  p->current_position = ftello(p->file);
}

size_t
mm_file_io_c::_write(const void *buffer,
                     size_t size) {
  auto p          = p_func();
  size_t bwritten = fwrite(buffer, 1, size, p->file);
  if (ferror(p->file) != 0)
    throw mtx::mm_io::read_write_x{mtx::mm_io::make_error_code()};

  p->current_position += bwritten;
  p->cached_size       = -1;

  return bwritten;
}

uint32_t
mm_file_io_c::_read(void *buffer,
                    size_t size) {
  auto p        = p_func();
  int64_t bread = fread(buffer, 1, size, p->file);

  p->current_position += bread;

  return bread;
}

void
mm_file_io_c::close() {
  auto p = p_func();

  if (p->file) {
    if (mm_file_io_private_c::ms_flush_on_close && (p->mode != libebml::MODE_READ))
      fflush(p->file);

    fclose(p->file);
    p->file = nullptr;
  }

  p->file_name.clear();
}

bool
mm_file_io_c::eof() {
  return feof(p_func()->file) != 0;
}

void
mm_file_io_c::clear_eof() {
  clearerr(p_func()->file);
}

int
mm_file_io_c::truncate(int64_t pos) {
  auto p         = p_func();
  p->cached_size = -1;
  return ftruncate(fileno(p->file), pos);
}
