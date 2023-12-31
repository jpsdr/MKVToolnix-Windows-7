/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   byte swapping functions
*/

#include "common/common_pch.h"

#include <stdexcept>

#include "common/bswap.h"
#include "common/endian.h"

namespace mtx::bytes {

void
swap_buffer(uint8_t const *src,
            uint8_t *dst,
            std::size_t num_bytes,
            std::size_t word_length) {
  if ((num_bytes % word_length) != 0)
    throw std::invalid_argument(fmt::format(Y("The number of bytes to swap isn't divisible by {0}."), word_length));

  for (int idx = 0; idx < static_cast<int>(num_bytes); idx += word_length)
    put_uint_le(&dst[idx], get_uint_be(&src[idx], word_length), word_length);
}

}
