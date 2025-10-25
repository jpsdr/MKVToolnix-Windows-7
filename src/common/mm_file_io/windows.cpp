/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   IO callback class implementation (Windows specific parts)

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <direct.h>
#include <fcntl.h>
#include <errno.h>
#include <io.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "common/endian.h"
#include "common/error.h"
#include "common/fs_sys_helpers.h"
#include "common/mm_io_p.h"
#include "common/mm_io_x.h"
#include "common/mm_file_io.h"
#include "common/mm_file_io_p.h"
#include "common/path.h"
#include "common/strings/editing.h"
#include "common/strings/parsing.h"
#include "common/strings/utf8.h"

mm_file_io_private_c::mm_file_io_private_c(std::string const &p_file_name,
                                           libebml::open_mode const p_mode)
  : file_name{p_file_name}
  , mode{p_mode}
{
  DWORD access_mode, share_mode, disposition;

  switch (mode) {
    case libebml::MODE_READ:
      access_mode = GENERIC_READ;
      share_mode  = FILE_SHARE_READ | FILE_SHARE_WRITE;
      disposition = OPEN_EXISTING;
      break;
    case libebml::MODE_WRITE:
      access_mode = GENERIC_WRITE | GENERIC_READ;
      share_mode  = FILE_SHARE_READ;
      disposition = OPEN_EXISTING;
      break;
    case libebml::MODE_SAFE:
      access_mode = GENERIC_WRITE | GENERIC_READ;
      share_mode  = FILE_SHARE_READ;
      disposition = OPEN_ALWAYS;
      break;
    case libebml::MODE_CREATE:
      access_mode = GENERIC_WRITE | GENERIC_READ;
      share_mode  = FILE_SHARE_READ;
      disposition = CREATE_ALWAYS;
      break;
    default:
      throw mtx::invalid_parameter_x();
  }

  if ((libebml::MODE_WRITE == mode) || (libebml::MODE_CREATE == mode))
    mm_file_io_c::prepare_path(file_name);

  auto w_file_name = to_wide(file_name);
  file             = CreateFileW(w_file_name.c_str(), access_mode, share_mode, nullptr, disposition, 0, nullptr);
  if (file == INVALID_HANDLE_VALUE)
    throw mtx::mm_io::open_x{mtx::mm_io::make_error_code()};

  dos_style_newlines = true;
}

void
mm_file_io_c::close() {
  auto p = p_func();

  if (p->file) {
    if (mm_file_io_private_c::ms_flush_on_close && (p->mode != libebml::MODE_READ))
      FlushFileBuffers(p->file);

    CloseHandle(p->file);
    p->file = INVALID_HANDLE_VALUE;
  }
  p->file_name.clear();
}

uint64_t
mm_file_io_c::get_real_file_pointer() {
  auto p    = p_func();
  LONG high = 0;
  DWORD low = SetFilePointer(p->file, 0, &high, FILE_CURRENT);

  if ((low == INVALID_SET_FILE_POINTER) && (GetLastError() != NO_ERROR))
    return (uint64_t)-1;

  return (((uint64_t)high) << 32) | (uint64_t)low;
}

void
mm_file_io_c::setFilePointer(int64_t offset,
                             libebml::seek_mode mode) {
  auto p       = p_func();
  DWORD method = libebml::seek_beginning == mode ? FILE_BEGIN
               : libebml::seek_current   == mode ? FILE_CURRENT
               : libebml::seek_end       == mode ? FILE_END
               :                                   FILE_BEGIN;
  LONG high    = (LONG)(offset >> 32);
  DWORD low    = SetFilePointer(p->file, (LONG)(offset & 0xffffffff), &high, method);

  if ((INVALID_SET_FILE_POINTER == low) && (GetLastError() != NO_ERROR))
    throw mtx::mm_io::seek_x{mtx::mm_io::make_error_code()};

  p->eof              = false;
  p->current_position = (int64_t)low + ((int64_t)high << 32);
}

uint32_t
mm_file_io_c::_read(void *buffer,
                    size_t size) {
  auto p = p_func();

  DWORD bytes_read;

  if (!ReadFile(p->file, buffer, size, &bytes_read, nullptr)) {
    p->eof              = true;
    p->current_position = get_real_file_pointer();

    return 0;
  }

  p->eof               = size != bytes_read;
  p->current_position += bytes_read;

  return bytes_read;
}

size_t
mm_file_io_c::_write(const void *buffer,
                     size_t size) {
  auto p = p_func();

  DWORD bytes_written;

  if (!WriteFile(p->file, buffer, size, &bytes_written, nullptr))
    bytes_written = 0;

  if (bytes_written != size) {
    auto error          = GetLastError();
    auto error_msg_utf8 = mtx::sys::format_windows_message(error);
    mxerror(fmt::format(FY("Could not write to the destination file: {0} ({1})\n"), error, error_msg_utf8));
  }

  p->current_position += bytes_written;
  p->cached_size       = -1;
  p->eof               = false;

  return bytes_written;
}

bool
mm_file_io_c::eof() {
  return p_func()->eof;
}

void
mm_file_io_c::clear_eof() {
  p_func()->eof = false;
}

int
mm_file_io_c::truncate(int64_t pos) {
  auto p = p_func();

  p->cached_size = -1;

  save_pos();
  if (setFilePointer2(pos)) {
    bool result = SetEndOfFile(p->file);
    restore_pos();

    return result ? 0 : -1;
  }

  restore_pos();

  return -1;
}
