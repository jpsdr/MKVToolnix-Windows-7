/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   random number generating functions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <cstdlib>
#include <random>

#include "common/fs_sys_helpers.h"
#include "common/random.h"

namespace {

std::unique_ptr<std::mt19937_64> s_generator;
std::uniform_int_distribution<uint8_t> s_uint8_distribution;
std::uniform_int_distribution<uint32_t> s_uint32_distribution;
std::uniform_int_distribution<uint64_t> s_uint64_distribution;

}

void
random_c::init(std::optional<uint64_t> seed) {
  s_generator = std::make_unique<std::mt19937_64>();

  if (!seed) {
#if defined(SYS_WINDOWS)
    // mingw's implementation of std::random_device yields deterministic
    // sequences, unfortunately.
    seed = mtx::sys::get_current_time_millis();

#else
    seed = std::random_device()();
#endif
  }

  std::srand(*seed);
  s_generator->seed(*seed);
}

void
random_c::generate_bytes(void *destination,
                         size_t num_bytes) {
  auto dst = static_cast<uint8_t *>(destination);

  for (int i = 0; i < static_cast<int>(num_bytes); ++i, ++dst)
    *dst = s_uint8_distribution(*s_generator);
}

uint8_t
random_c::generate_8bits() {
  return s_uint8_distribution(*s_generator);
}

uint32_t
random_c::generate_32bits() {
  return s_uint32_distribution(*s_generator);
}

uint64_t
random_c::generate_64bits() {
  return s_uint64_distribution(*s_generator);
}
