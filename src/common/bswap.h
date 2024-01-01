/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   byte swapping functions

   This code for bswap_16/32/64 was taken from the ffmpeg project,
   files "libavutil/bswap.h".
*/

#pragma once

#include "common/common_pch.h"

namespace mtx::bytes {

inline uint16_t
swap_16(uint16_t x) {
  return (x >> 8) | (x << 8);
}

inline uint32_t
swap_32(uint32_t x) {
  x = ((x <<  8) & 0xff00ff00) | ((x >>  8) & 0x00ff00ff);
  x =  (x >> 16)               |  (x << 16);

  return x;
}

inline uint64_t
swap_64(uint64_t x) {
  union {
    uint64_t ll;
    uint32_t l[2];
  } w, r;

  w.ll   = x;
  r.l[0] = swap_32(w.l[1]);
  r.l[1] = swap_32(w.l[0]);

  return r.ll;
}

void swap_buffer(uint8_t const *src, uint8_t *dst, std::size_t num_bytes, std::size_t word_length);

}
