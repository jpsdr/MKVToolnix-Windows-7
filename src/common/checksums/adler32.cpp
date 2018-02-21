/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   checksum calculations – Adler-32 implementation

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/checksums/adler32.h"
#include "common/endian.h"

namespace mtx { namespace checksum {

adler32_c::adler32_c()
  : m_a{1}
  , m_b{0}
{
}

adler32_c::~adler32_c() {
}

memory_cptr
adler32_c::get_result()
  const {
  auto result = (m_b << 16) | m_a;
  unsigned char buf[4];

  put_uint32_be(buf, result);

  return memory_c::clone(buf, 4);
}

uint64_t
adler32_c::get_result_as_uint()
  const {
  return (m_b << 16) | m_a;
}

void
adler32_c::add_impl(unsigned char const *buffer,
                    size_t size) {
  for (auto idx = 0u; idx < size; ++idx) {
    m_a = (m_a + buffer[idx]) % msc_mod_adler;
    m_b = (m_b + m_a)         % msc_mod_adler;
  }
}

}} // namespace mtx { namespace checksum {
