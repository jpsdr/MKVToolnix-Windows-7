/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   checksum calculations – MD5 definition

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/checksums/base.h"

namespace mtx { namespace checksum {

class md5_c: public base_c {
protected:
  uint32_t m_a, m_b, m_c, m_d;
  uint64_t m_size;
  unsigned char m_buffer[64], m_result[16];
  uint32_t m_block[16];

public:
  md5_c();
  virtual ~md5_c();

  virtual memory_cptr get_result() const;
  virtual base_c &finish();

protected:
  virtual void add_impl(unsigned char const *buffer, size_t size);
  unsigned char const *work(unsigned char const *data, size_t size);
};

}} // namespace mtx { namespace checksum {
