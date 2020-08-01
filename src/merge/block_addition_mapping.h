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

struct block_addition_mapping_t {
  std::optional<uint64_t> id_type, id_value;
  memory_cptr id_extra_data;

  bool is_valid() const {
    return id_type || id_value || (id_extra_data && id_extra_data->get_size());
  }
};
