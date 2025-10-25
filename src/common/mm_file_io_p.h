/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#if defined(SYS_WINDOWS)
# include <windows.h>
#endif

#include "common/mm_io_p.h"

class mm_file_io_c;

class mm_file_io_private_c : public mm_io_private_c {
public:
  std::string file_name;
  libebml::open_mode mode{libebml::MODE_READ};

#if defined(SYS_WINDOWS)
  bool eof{};
  HANDLE file{};
#else
  FILE *file{};
#endif

  explicit mm_file_io_private_c(std::string const &p_file_name, libebml::open_mode const p_mode);

public:
  static bool ms_flush_on_close;
};
