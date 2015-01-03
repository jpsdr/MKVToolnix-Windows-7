/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   checksum calculations – base class definitions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef MTX_COMMON_CHECKSUMS_BASE_H
#define MTX_COMMON_CHECKSUMS_BASE_H

#include "common/common_pch.h"

namespace mtx { namespace checksum {

enum algorithm_e {
    adler32
  , crc8_atm
  , crc16_ansi
  , crc16_ccitt
  , crc32_ieee
  , crc32_ieee_le
  , md5
};

class base_c;
typedef std::shared_ptr<base_c> base_cptr;

class base_c {
public:
  base_c();
  virtual ~base_c();

  base_c &add(void const *buffer, size_t size);
  base_c &add(memory_c const &buffer);

  virtual base_c &finish();
  virtual memory_cptr get_result() const = 0;

protected:
  virtual void add_impl(unsigned char const *buffer, size_t size) = 0;
};

class set_initial_value_c {
public:
  virtual ~set_initial_value_c();
  virtual void set_initial_value(uint64_t initial_value);
  virtual void set_initial_value(memory_c const &initial_value);
  virtual void set_initial_value(unsigned char const *buffer, size_t size);

protected:
  virtual void set_initial_value_impl(uint64_t initial_value) = 0;
  virtual void set_initial_value_impl(unsigned char const *buffer, size_t size) = 0;
};

class uint_result_c {
public:
  virtual ~uint_result_c();
  virtual uint64_t get_result_as_uint() const = 0;
};

base_cptr for_algorithm(algorithm_e algorithm, uint64_t initial_value = 0);
memory_cptr calculate(algorithm_e algorithm, memory_c const &buffer, uint64_t initial_value = 0);
memory_cptr calculate(algorithm_e algorithm, void const *buffer, size_t size, uint64_t initial_value = 0);
uint64_t calculate_as_uint(algorithm_e algorithm, memory_c const &buffer, uint64_t initial_value = 0);
uint64_t calculate_as_uint(algorithm_e algorithm, void const *buffer, size_t size, uint64_t initial_value = 0);

}} // namespace mtx { namespace checksum {

#endif // MTX_COMMON_CHECKSUMS_BASE_H
