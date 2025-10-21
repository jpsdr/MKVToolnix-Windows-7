/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   checksum calculations – definitions for CRC variations

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/checksums/base.h"

namespace mtx::checksum {

class crc_base_c: public base_c, public uint_result_c, public set_initial_value_c {
protected:
  enum type_e {
    crc_8_atm      = 0,
    crc_16_ansi    = 1,
    crc_16_ccitt   = 2,
    crc_32_ieee    = 3,
    crc_32_ieee_le = 4,
    crc_16_002d    = 5,
  };

  using table_t = std::vector<uint32_t>;

  struct table_parameters_t {
    uint8_t  le;
    uint8_t  bits;
    uint32_t poly;
  };

  static table_parameters_t const ms_table_parameters[6];

protected:
  type_e m_type;
  table_t &m_table;
  uint32_t m_crc;
  uint64_t m_xor_result;
  bool m_result_in_le;

protected:
  crc_base_c(type_e type, table_t &table, uint32_t crc);

  void init_table();

public:
  virtual ~crc_base_c() = default;

  virtual memory_cptr get_result() const;
  virtual uint64_t get_result_as_uint() const;

  virtual void set_xor_result(uint64_t xor_result);
  virtual void set_result_in_le(bool result_in_le);

protected:
  virtual void add_impl(uint8_t const *buffer, size_t size);

  virtual void set_initial_value_impl(uint64_t initial_value) ;
  virtual void set_initial_value_impl(uint8_t const *buffer, size_t size);
};

class crc8_atm_c: public crc_base_c {
protected:
  static table_t ms_table;

public:
  crc8_atm_c(uint32_t initial_value = 0);
  virtual ~crc8_atm_c() = default;
};

class crc16_ansi_c: public crc_base_c {
protected:
  static table_t ms_table;

public:
  crc16_ansi_c(uint32_t initial_value = 0);
  virtual ~crc16_ansi_c() = default;
};

class crc16_ccitt_c: public crc_base_c {
protected:
  static table_t ms_table;

public:
  crc16_ccitt_c(uint32_t initial_value = 0);
  virtual ~crc16_ccitt_c() = default;
};

class crc16_002d_c: public crc_base_c {
protected:
  static table_t ms_table;

public:
  crc16_002d_c(uint32_t initial_value = 0);
  virtual ~crc16_002d_c() = default;
};

class crc32_ieee_c: public crc_base_c {
protected:
  static table_t ms_table;

public:
  crc32_ieee_c(uint32_t initial_value = 0);
  virtual ~crc32_ieee_c() = default;
};

class crc32_ieee_le_c: public crc_base_c {
protected:
  static table_t ms_table;

public:
  crc32_ieee_le_c(uint32_t initial_value = 0);
  virtual ~crc32_ieee_le_c() = default;
};

} // namespace mtx::checksum
