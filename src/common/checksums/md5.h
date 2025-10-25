/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   checksum calculations – MD5 definition

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/checksums/base.h"

namespace mtx::checksum {

class md5_c: public base_c {
protected:
  uint32_t m_a, m_b, m_c, m_d;
  uint64_t m_size;
  uint8_t m_buffer[64], m_result[16];
  uint32_t m_block[16];

public:
  md5_c();
  virtual ~md5_c() = default;

  virtual memory_cptr get_result() const;
  virtual base_c &finish();

protected:
  virtual void add_impl(uint8_t const *buffer, size_t size);
  uint8_t const *work(uint8_t const *data, size_t size);
};

} // namespace mtx::checksum
