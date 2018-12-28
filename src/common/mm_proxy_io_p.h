/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/mm_io_p.h"

class mm_proxy_io_c;

class mm_proxy_io_private_c : public mm_io_private_c {
public:
  mm_io_cptr proxy_io;

  explicit mm_proxy_io_private_c(mm_io_cptr const &p_proxy_io)
    : proxy_io{p_proxy_io}
  {
  }
};
