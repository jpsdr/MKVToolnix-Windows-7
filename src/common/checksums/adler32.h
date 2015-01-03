/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   checksum calculations – Adler32 definition

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef MTX_COMMON_CHECKSUMS_ADLER32_H
#define MTX_COMMON_CHECKSUMS_ADLER32_H

#include "common/common_pch.h"

#include "common/checksums/base.h"

namespace mtx { namespace checksum {

class adler32_c: public base_c, public uint_result_c {
protected:
  uint32_t const mc_mod_adler = 65521;
  uint32_t m_a, m_b;

public:
  adler32_c();
  virtual ~adler32_c();

  virtual memory_cptr get_result() const;
  virtual uint64_t get_result_as_uint() const;

protected:
  virtual void add_impl(unsigned char const *buffer, size_t size);
};

}} // namespace mtx { namespace checksum {

#endif // MTX_COMMON_CHECKSUMS_ADLER32_H
