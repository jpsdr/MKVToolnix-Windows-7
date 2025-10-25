/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   checksum calculations – MD5 implementation

   Adopted by Moritz Bunkus <mo@bunkus.online> from the Public Domain
   source code available at
   http://openwall.info/wiki/people/solar/software/public-domain-source-code/md5
*/

#include "common/common_pch.h"

#include "common/checksums/md5.h"
#include "common/endian.h"

namespace mtx::checksum {

#define F(x,  y, z) ((z)  ^ ((x) & ((y) ^ (z))))
#define G(x,  y, z) ((y)  ^ ((z) & ((x) ^ (y))))
#define H(x,  y, z) (((x) ^ (y)) ^ (z))
#define H2(x, y, z) ((x)  ^ ((y) ^ (z)))
#define I(x,  y, z) ((y)  ^ ((x) | ~(z)))

// The MD5 transformation for all four rounds.
#define STEP(f, a, b, c, d, x, t, s)                          \
  (a) += f((b), (c), (d)) + (x) + (t);                        \
  (a)  = (((a) << (s)) | (((a) & 0xffffffff) >> (32 - (s)))); \
  (a) += (b);

// SET reads 4 input bytes in little-endian byte order and stores them
// in a properly aligned word in host byte order.
#define SET(n) (m_block[(n)]) = get_uint32_le(&data[(n) * 4])
#define GET(n) (m_block[(n)])

md5_c::md5_c()
  : m_a{0x67452301}
  , m_b{0xefcdab89}
  , m_c{0x98badcfe}
  , m_d{0x10325476}
  , m_size{}
{
}

// This processes one or more 64-byte data blocks, but does NOT update
// the bit counters.  There are no alignment requirements.
uint8_t const *
md5_c::work(uint8_t const *data,
            size_t size) {
  auto a = m_a;
  auto b = m_b;
  auto c = m_c;
  auto d = m_d;

  do {
    auto saved_a = a;
    auto saved_b = b;
    auto saved_c = c;
    auto saved_d = d;

    // Round 1
    STEP(F, a, b, c, d, SET(0), 0xd76aa478, 7);
    STEP(F, d, a, b, c, SET(1), 0xe8c7b756, 12);
    STEP(F, c, d, a, b, SET(2), 0x242070db, 17);
    STEP(F, b, c, d, a, SET(3), 0xc1bdceee, 22);
    STEP(F, a, b, c, d, SET(4), 0xf57c0faf, 7);
    STEP(F, d, a, b, c, SET(5), 0x4787c62a, 12);
    STEP(F, c, d, a, b, SET(6), 0xa8304613, 17);
    STEP(F, b, c, d, a, SET(7), 0xfd469501, 22);
    STEP(F, a, b, c, d, SET(8), 0x698098d8, 7);
    STEP(F, d, a, b, c, SET(9), 0x8b44f7af, 12);
    STEP(F, c, d, a, b, SET(10), 0xffff5bb1, 17);
    STEP(F, b, c, d, a, SET(11), 0x895cd7be, 22);
    STEP(F, a, b, c, d, SET(12), 0x6b901122, 7);
    STEP(F, d, a, b, c, SET(13), 0xfd987193, 12);
    STEP(F, c, d, a, b, SET(14), 0xa679438e, 17);
    STEP(F, b, c, d, a, SET(15), 0x49b40821, 22);

    // Round 2
    STEP(G, a, b, c, d, GET(1), 0xf61e2562, 5);
    STEP(G, d, a, b, c, GET(6), 0xc040b340, 9);
    STEP(G, c, d, a, b, GET(11), 0x265e5a51, 14);
    STEP(G, b, c, d, a, GET(0), 0xe9b6c7aa, 20);
    STEP(G, a, b, c, d, GET(5), 0xd62f105d, 5);
    STEP(G, d, a, b, c, GET(10), 0x02441453, 9);
    STEP(G, c, d, a, b, GET(15), 0xd8a1e681, 14);
    STEP(G, b, c, d, a, GET(4), 0xe7d3fbc8, 20);
    STEP(G, a, b, c, d, GET(9), 0x21e1cde6, 5);
    STEP(G, d, a, b, c, GET(14), 0xc33707d6, 9);
    STEP(G, c, d, a, b, GET(3), 0xf4d50d87, 14);
    STEP(G, b, c, d, a, GET(8), 0x455a14ed, 20);
    STEP(G, a, b, c, d, GET(13), 0xa9e3e905, 5);
    STEP(G, d, a, b, c, GET(2), 0xfcefa3f8, 9);
    STEP(G, c, d, a, b, GET(7), 0x676f02d9, 14);
    STEP(G, b, c, d, a, GET(12), 0x8d2a4c8a, 20);

    // Round 3
    STEP(H, a, b, c, d, GET(5), 0xfffa3942, 4);
    STEP(H2, d, a, b, c, GET(8), 0x8771f681, 11);
    STEP(H, c, d, a, b, GET(11), 0x6d9d6122, 16);
    STEP(H2, b, c, d, a, GET(14), 0xfde5380c, 23);
    STEP(H, a, b, c, d, GET(1), 0xa4beea44, 4);
    STEP(H2, d, a, b, c, GET(4), 0x4bdecfa9, 11);
    STEP(H, c, d, a, b, GET(7), 0xf6bb4b60, 16);
    STEP(H2, b, c, d, a, GET(10), 0xbebfbc70, 23);
    STEP(H, a, b, c, d, GET(13), 0x289b7ec6, 4);
    STEP(H2, d, a, b, c, GET(0), 0xeaa127fa, 11);
    STEP(H, c, d, a, b, GET(3), 0xd4ef3085, 16);
    STEP(H2, b, c, d, a, GET(6), 0x04881d05, 23);
    STEP(H, a, b, c, d, GET(9), 0xd9d4d039, 4);
    STEP(H2, d, a, b, c, GET(12), 0xe6db99e5, 11);
    STEP(H, c, d, a, b, GET(15), 0x1fa27cf8, 16);
    STEP(H2, b, c, d, a, GET(2), 0xc4ac5665, 23);

    // Round 4
    STEP(I, a, b, c, d, GET(0), 0xf4292244, 6);
    STEP(I, d, a, b, c, GET(7), 0x432aff97, 10);
    STEP(I, c, d, a, b, GET(14), 0xab9423a7, 15);
    STEP(I, b, c, d, a, GET(5), 0xfc93a039, 21);
    STEP(I, a, b, c, d, GET(12), 0x655b59c3, 6);
    STEP(I, d, a, b, c, GET(3), 0x8f0ccc92, 10);
    STEP(I, c, d, a, b, GET(10), 0xffeff47d, 15);
    STEP(I, b, c, d, a, GET(1), 0x85845dd1, 21);
    STEP(I, a, b, c, d, GET(8), 0x6fa87e4f, 6);
    STEP(I, d, a, b, c, GET(15), 0xfe2ce6e0, 10);
    STEP(I, c, d, a, b, GET(6), 0xa3014314, 15);
    STEP(I, b, c, d, a, GET(13), 0x4e0811a1, 21);
    STEP(I, a, b, c, d, GET(4), 0xf7537e82, 6);
    STEP(I, d, a, b, c, GET(11), 0xbd3af235, 10);
    STEP(I, c, d, a, b, GET(2), 0x2ad7d2bb, 15);
    STEP(I, b, c, d, a, GET(9), 0xeb86d391, 21);

    a    += saved_a;
    b    += saved_b;
    c    += saved_c;
    d    += saved_d;

    data += 64;

  } while (size -= 64);

  m_a = a;
  m_b = b;
  m_c = c;
  m_d = d;

  return data;
}

void
md5_c::add_impl(uint8_t const *data,
                size_t size) {
  auto saved_size  = m_size;
  m_size          += size;
  auto used        = saved_size & 0x3f;

  if (used) {
    auto available = 64 - used;

    if (size < available) {
      std::memcpy(&m_buffer[used], data, size);
      return;
    }

    std::memcpy(&m_buffer[used], data, available);
    data += available;
    size -= available;

    work(m_buffer, 64);
  }

  if (size >= 64) {
    data  = work(data, size & ~0x3full);
    size &= 0x3f;
  }

  std::memcpy(m_buffer, data, size);
}

base_c &
md5_c::finish() {
  auto used        = m_size & 0x3f;

  m_buffer[used++] = 0x80;

  auto available   = 64 - used;

  if (available < 8) {
    std::memset(&m_buffer[used], 0, available);
    work(m_buffer, 64);
    used      = 0;
    available = 64;
  }

  std::memset(&m_buffer[used], 0, available - 8);

  put_uint64_le(&m_buffer[56], m_size << 3);

  work(m_buffer, 64);

  put_uint32_le(&m_result[0],  m_a);
  put_uint32_le(&m_result[4],  m_b);
  put_uint32_le(&m_result[8],  m_c);
  put_uint32_le(&m_result[12], m_d);

  return *this;
}

memory_cptr
md5_c::get_result()
  const {
  return memory_c::clone(m_result, 16);
}

} // namespace mtx::checksum
