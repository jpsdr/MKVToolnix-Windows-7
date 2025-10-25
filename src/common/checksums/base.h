/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   checksum calculations – base class definitions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/checksums/base_fwd.h"

namespace mtx::checksum {

class base_c {
public:
  base_c() = default;
  virtual ~base_c() = default;

  base_c &add(void const *buffer, size_t size);
  base_c &add(memory_c const &buffer);

  virtual base_c &finish();
  virtual memory_cptr get_result() const = 0;

protected:
  virtual void add_impl(uint8_t const *buffer, size_t size) = 0;
};

class set_initial_value_c {
public:
  virtual ~set_initial_value_c() = default;
  virtual void set_initial_value(uint64_t initial_value);
  virtual void set_initial_value(memory_c const &initial_value);
  virtual void set_initial_value(uint8_t const *buffer, size_t size);

protected:
  virtual void set_initial_value_impl(uint64_t initial_value) = 0;
  virtual void set_initial_value_impl(uint8_t const *buffer, size_t size) = 0;
};

class uint_result_c {
public:
  virtual ~uint_result_c() = default;
  virtual uint64_t get_result_as_uint() const = 0;
};

} // namespace mtx::checksum
