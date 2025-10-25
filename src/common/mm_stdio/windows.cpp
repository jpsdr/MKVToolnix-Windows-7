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

#include <fcntl.h>
#include <io.h>
#include <windows.h>

#include "common/mm_io_p.h"
#include "common/mm_stdio.h"
#include "common/strings/utf8.h"

static bool s_stdout_binmode_set = false;

size_t
mm_stdio_c::_write(const void *buffer,
                   size_t size) {
  HANDLE h_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
  if (INVALID_HANDLE_VALUE == h_stdout)
    return 0;

  DWORD file_type = GetFileType(h_stdout);
  bool is_console = false;
  if ((FILE_TYPE_UNKNOWN != file_type) && ((file_type & ~FILE_TYPE_REMOTE) == FILE_TYPE_CHAR)) {
    DWORD dummy;
    is_console = GetConsoleMode(h_stdout, &dummy);
  }

  if (is_console) {
    const std::wstring &w = to_wide(std::string(static_cast<const char *>(buffer), size));
    DWORD bytes_written   = 0;

    WriteConsoleW(h_stdout, w.c_str(), w.length(), &bytes_written, nullptr);

    return bytes_written;
  }

  if (!s_stdout_binmode_set) {
    _setmode(1, _O_BINARY);
    s_stdout_binmode_set = true;
  }

  size_t bytes_written = fwrite(buffer, 1, size, stdout);
  fflush(stdout);

  p_func()->cached_size = -1;

  return bytes_written;
}
