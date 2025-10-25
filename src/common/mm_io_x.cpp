/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Exceptios for the I/O callback class

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#if defined(SYS_WINDOWS)
# include <windows.h>
#else
# include <cerrno>
#endif

#include "common/mm_io_x.h"

namespace mtx::mm_io {

std::error_code
make_error_code() {
#ifdef SYS_WINDOWS
  return { static_cast<int>(::GetLastError()), std::system_category() };
#else
  return { errno, std::generic_category() };
#endif
}

std::string
exception::error()
  const noexcept {
  return std::errc::no_such_file_or_directory == code() ? Y("The file or directory was not found")
       : std::errc::no_space_on_device        == code() ? Y("No space left to write to")
       : std::errc::permission_denied         == code() ? Y("No permission to read from, to write to or to create")
       :                                                  code().message();
}

}
