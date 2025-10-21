/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   checksum calculations – Adler-32 implementation

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/checksums/adler32.h"
#include "common/endian.h"

namespace mtx::checksum {

adler32_c::adler32_c()
  : m_a{1}
  , m_b{0}
{
}

memory_cptr
adler32_c::get_result()
  const {
  auto result = (m_b << 16) | m_a;
  uint8_t buf[4];

  put_uint32_be(buf, result);

  return memory_c::clone(buf, 4);
}

uint64_t
adler32_c::get_result_as_uint()
  const {
  return (m_b << 16) | m_a;
}

void
adler32_c::add_impl(uint8_t const *buffer,
                    size_t size) {
  for (auto idx = 0u; idx < size; ++idx) {
    m_a = (m_a + buffer[idx]) % msc_mod_adler;
    m_b = (m_b + m_a)         % msc_mod_adler;
  }
}

} // namespace mtx::checksum
