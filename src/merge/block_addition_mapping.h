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

struct block_addition_mapping_t {
  std::string id_name;
  std::optional<uint64_t> id_type, id_value;
  memory_cptr id_extra_data;

  bool is_valid() const {
    return !!id_type;
  }
};
