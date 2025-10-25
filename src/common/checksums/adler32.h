/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   checksum calculations – Adler-32 definition

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/checksums/base.h"

namespace mtx::checksum {

class adler32_c: public base_c, public uint_result_c {
protected:
  static uint32_t const msc_mod_adler = 65521;
  uint32_t m_a, m_b;

public:
  adler32_c();
  virtual ~adler32_c() = default;

  virtual memory_cptr get_result() const;
  virtual uint64_t get_result_as_uint() const;

protected:
  virtual void add_impl(uint8_t const *buffer, size_t size);
};

} // namespace mtx::checksum
