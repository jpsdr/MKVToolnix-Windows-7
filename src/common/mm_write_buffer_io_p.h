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

#include "common/mm_proxy_io_p.h"

class mm_write_buffer_io_c;

class mm_write_buffer_io_private_c : public mm_proxy_io_private_c {
public:
  memory_cptr af_buffer;
  uint8_t *buffer{};
  std::size_t fill{};
  std::size_t const size{};

  explicit mm_write_buffer_io_private_c(mm_io_cptr const &p_proxy_io,
                                        std::size_t p_buffer_size)
    : mm_proxy_io_private_c{p_proxy_io}
    , af_buffer{memory_c::alloc(p_buffer_size)}
    , buffer{af_buffer->get_buffer()}
    , size{p_buffer_size}
  {
  }
};
