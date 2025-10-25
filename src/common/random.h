/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   definitions for random number generating functions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

class random_c {
public:
  static void init(std::optional<uint64_t> seed = std::nullopt);

  static void generate_bytes(void *destination, size_t num_bytes);
  static uint8_t generate_8bits();
  static uint32_t generate_32bits();
  static uint64_t generate_64bits();
};
