/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   checksum calculations – base class definitions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/checksums/base_fwd.h"

namespace mtx { namespace checksum {

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

}} // namespace mtx { namespace checksum {
