/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   byte swapping functions
*/

#include "common/common_pch.h"

#include <stdexcept>

#include "common/bswap.h"
#include "common/endian.h"

namespace mtx {

void
bswap_buffer_16(unsigned char const *src,
                unsigned char *dst,
                std::size_t num_bytes) {
  if ((num_bytes % 2) != 0)
    throw std::invalid_argument((boost::format(Y("The number of bytes to swap isn't divisible by %1%.")) % 2).str());

  for (std::size_t idx = 0; idx < num_bytes; idx += 2)
    put_uint16_le(&dst[idx], get_uint16_be(&src[idx]));
}

void
bswap_buffer_32(unsigned char const *src,
                unsigned char *dst,
                std::size_t num_bytes) {
  if ((num_bytes % 4) != 0)
    throw std::invalid_argument((boost::format(Y("The number of bytes to swap isn't divisible by %1%.")) % 4).str());

  for (std::size_t idx = 0; idx < num_bytes; idx += 4)
    put_uint32_le(&dst[idx], get_uint32_be(&src[idx]));
}

void
bswap_buffer_64(unsigned char const *src,
                unsigned char *dst,
                std::size_t num_bytes) {
  if ((num_bytes % 8) != 0)
    throw std::invalid_argument((boost::format(Y("The number of bytes to swap isn't divisible by %1%.")) % 8).str());

  for (std::size_t idx = 0; idx < num_bytes; idx += 8)
    put_uint64_le(&dst[idx], get_uint64_be(&src[idx]));
}

}
