/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Endian helper functions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <algorithm>

#include "common/endian.h"

uint16_t
get_uint16_le(const void *buf) {
  return get_uint_le(buf, 2);
}

uint32_t
get_uint24_le(const void *buf) {
  return get_uint_le(buf, 3);
}

uint32_t
get_uint32_le(const void *buf) {
  return get_uint_le(buf, 4);
}

uint64_t
get_uint64_le(const void *buf) {
  return get_uint_le(buf, 8);
}

uint64_t
get_uint_le(const void *buf,
            int num_bytes) {
  int i;
  num_bytes    = std::min(std::max(1, num_bytes), 8);
  auto tmp     = static_cast<uint8_t const *>(buf);
  uint64_t ret = 0;
  for (i = num_bytes - 1; 0 <= i; --i)
    ret = (ret << 8) + (tmp[i] & 0xff);

  return ret;
}

uint16_t
get_uint16_be(const void *buf) {
  return get_uint_be(buf, 2);
}

uint32_t
get_uint24_be(const void *buf) {
  return get_uint_be(buf, 3);
}

uint32_t
get_uint32_be(const void *buf) {
  return get_uint_be(buf, 4);
}

uint64_t
get_uint64_be(const void *buf) {
  return get_uint_be(buf, 8);
}

uint64_t
get_uint_be(const void *buf,
            int num_bytes) {
  int i;
  num_bytes    = std::min(std::max(1, num_bytes), 8);
  auto tmp     = static_cast<uint8_t const *>(buf);
  uint64_t ret = 0;
  for (i = 0; num_bytes > i; ++i)
    ret = (ret << 8) + (tmp[i] & 0xff);

  return ret;
}

void
put_uint_le(void *buf,
            uint64_t value,
            size_t num_bytes) {
  auto tmp  = static_cast<uint8_t *>(buf);
  num_bytes = std::min<size_t>(std::max<size_t>(1, num_bytes), 8);

  for (auto idx = 0u; idx < num_bytes; ++idx) {
    tmp[idx] = value & 0xff;
    value  >>= 8;
  }
}

void
put_uint_be(void *buf,
            uint64_t value,
            size_t num_bytes) {
  auto tmp  = static_cast<uint8_t *>(buf);
  num_bytes = std::min<size_t>(std::max<size_t>(1, num_bytes), 8);

  for (auto idx = 0u; idx < num_bytes; ++idx) {
    tmp[num_bytes - idx - 1] = value & 0xff;
    value                  >>= 8;
  }
}

void
put_uint16_le(void *buf,
              uint16_t value) {
  put_uint_le(buf, value, 2);
}

void
put_uint24_le(void *buf,
              uint32_t value) {
  put_uint_le(buf, value, 3);
}

void
put_uint32_le(void *buf,
              uint32_t value) {
  put_uint_le(buf, value, 4);
}

void
put_uint64_le(void *buf,
              uint64_t value) {
  put_uint_le(buf, value, 8);
}

void
put_uint16_be(void *buf,
              uint16_t value) {
  put_uint_be(buf, value, 2);
}

void
put_uint24_be(void *buf,
              uint32_t value) {
  put_uint_be(buf, value, 3);
}

void
put_uint32_be(void *buf,
              uint32_t value) {
  put_uint_be(buf, value, 4);
}

void
put_uint64_be(void *buf,
              uint64_t value) {
  put_uint_be(buf, value, 8);
}
