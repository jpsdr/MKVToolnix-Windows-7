/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   checksum calculations – base class definitions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx::checksum {

enum class algorithm_e {
    adler32
  , crc8_atm
  , crc16_ansi
  , crc16_ccitt
  , crc16_002d
  , crc32_ieee
  , crc32_ieee_le
  , md5
};

class base_c;
using base_uptr = std::unique_ptr<base_c>;

class set_initial_value_c;
class uint_result_c;

base_uptr for_algorithm(algorithm_e algorithm, uint64_t initial_value = 0);
memory_cptr calculate(algorithm_e algorithm, memory_c const &buffer, uint64_t initial_value = 0);
memory_cptr calculate(algorithm_e algorithm, void const *buffer, size_t size, uint64_t initial_value = 0);
uint64_t calculate_as_uint(algorithm_e algorithm, memory_c const &buffer, uint64_t initial_value = 0);
uint64_t calculate_as_uint(algorithm_e algorithm, void const *buffer, size_t size, uint64_t initial_value = 0);
std::string calculate_as_hex_string(algorithm_e algorithm, memory_c const &buffer, uint64_t initial_value = 0);
std::string calculate_as_hex_string(algorithm_e algorithm, void const *buffer, size_t size, uint64_t initial_value = 0);

} // namespace mtx::checksum
