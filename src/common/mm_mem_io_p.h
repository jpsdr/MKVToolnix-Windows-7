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

#include "common/mm_io_p.h"

class mm_mem_io_c;

class mm_mem_io_private_c : public mm_io_private_c {
public:
  std::size_t pos{}, mem_size{}, allocated{}, increase{};
  uint8_t *mem{};
  const uint8_t *ro_mem{};
  bool free_mem{}, read_only{};
  std::string file_name;

  explicit mm_mem_io_private_c(uint8_t *p_mem,
                               uint64_t p_mem_size,
                               std::size_t p_increase)
    : mem_size{static_cast<std::size_t>(p_mem_size)}
    , allocated{static_cast<std::size_t>(p_mem_size)}
    , increase{p_increase}
    , mem{p_mem}
  {
    if (0 >= increase)
      throw mtx::invalid_parameter_x();

    if (!mem && (0 < increase)) {
      if (0 == mem_size)
        allocated = increase;

      mem      = safemalloc(allocated);
      free_mem = true;

    }
  }

  explicit mm_mem_io_private_c(uint8_t const *p_mem,
                               uint64_t p_mem_size)
    : mem_size{static_cast<std::size_t>(p_mem_size)}
    , allocated{static_cast<std::size_t>(p_mem_size)}
    , ro_mem{p_mem}
    , read_only{true}
  {
    if (!ro_mem)
      throw mtx::invalid_parameter_x();
  }

  explicit mm_mem_io_private_c(memory_c const &p_mem)
    : mem_size{p_mem.get_size()}
    , allocated{p_mem.get_size()}
    , ro_mem{p_mem.get_buffer()}
    , read_only{true}
  {
    if (!ro_mem)
      throw mtx::invalid_parameter_x{};
  }
};
