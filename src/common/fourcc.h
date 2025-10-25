/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   definitions and helper functions for FourCC handling

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <ostream>

#include "common/mm_io.h"

class fourcc_c {
public:
  enum class byte_order_e { big_endian, little_endian };

private:
  uint32_t m_value;

public:
  fourcc_c();

  // From an integer value:
  fourcc_c(uint32_t value, byte_order_e byte_order = byte_order_e::big_endian);

  // From strings:
  fourcc_c(std::string const &value);
  fourcc_c(char const *value);

  // From memory:
  fourcc_c(memory_cptr const &mem, byte_order_e byte_order = byte_order_e::big_endian);
  fourcc_c(uint8_t const *mem, byte_order_e byte_order = byte_order_e::big_endian);
  fourcc_c(uint32_t const *mem, byte_order_e byte_order = byte_order_e::big_endian);

  // From mm_io_c instances:
  fourcc_c(mm_io_cptr const &io, byte_order_e byte_order = byte_order_e::big_endian);
  fourcc_c(mm_io_c &io, byte_order_e byte_order = byte_order_e::big_endian);
  fourcc_c(mm_io_c *io, byte_order_e byte_order = byte_order_e::big_endian);

  fourcc_c &shift_read(mm_io_cptr const &io, byte_order_e byte_order = byte_order_e::big_endian);
  fourcc_c &shift_read(mm_io_c &io, byte_order_e byte_order = byte_order_e::big_endian);
  fourcc_c &shift_read(mm_io_c *io, byte_order_e byte_order = byte_order_e::big_endian);

  size_t write(memory_cptr const &mem, byte_order_e byte_order = byte_order_e::big_endian);
  size_t write(uint8_t *mem, byte_order_e byte_order = byte_order_e::big_endian);
  size_t write(mm_io_cptr const &io, byte_order_e byte_order = byte_order_e::big_endian);
  size_t write(mm_io_c &io, byte_order_e byte_order = byte_order_e::big_endian);
  size_t write(mm_io_c *io, byte_order_e byte_order = byte_order_e::big_endian);

  fourcc_c &reset();

  uint32_t value(byte_order_e byte_order = byte_order_e::big_endian) const;
  std::string str() const;
  std::string description() const;

  bool equiv(char const *cmp) const;
  bool equiv(std::vector<std::string> const &cmp) const;
  bool human_readable(size_t min_num = 3) const;

  explicit operator bool() const;
  bool operator ==(fourcc_c const &cmp) const;
  bool operator !=(fourcc_c const &cmp) const;

protected:
  // From memory & strings:
  static uint32_t read(void const *mem, byte_order_e byte_order);
  // From mm_io_c instances:
  static uint32_t read(mm_io_c &io, byte_order_e byte_order);
};

inline std::ostream &
operator <<(std::ostream &out,
            fourcc_c const &fourcc) {
  out << fourcc.str();
  return out;
}

#if FMT_VERSION >= 90000
template <> struct fmt::formatter<fourcc_c> : ostream_formatter {};
#endif  // FMT_VERSION >= 90000
