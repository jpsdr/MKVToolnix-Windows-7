/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   checksum calculations – CRC variations

   Written by Moritz Bunkus <mo@bunkus.online>.

   This code was adopted from the ffmpeg project, files
   "libavutil/crc.h" and "libavutil/crc.c".
*/

#include "common/common_pch.h"

#include "common/bswap.h"
#include "common/checksums/crc.h"
#include "common/endian.h"

namespace mtx::checksum {

crc_base_c::table_parameters_t const crc_base_c::ms_table_parameters[6] = {
  { 0,  8,       0x07 },
  { 0, 16,     0x8005 },
  { 0, 16,     0x1021 },
  { 0, 32, 0x04C11DB7 },
  { 1, 32, 0xEDB88320 },
  { 0, 16,     0x002d },
};

crc_base_c::crc_base_c(type_e type,
                       table_t &table,
                       uint32_t crc)
  : m_type{type}
  , m_table{table}
  , m_crc{crc}
  , m_xor_result{}
  , m_result_in_le{}
{
  if (m_table.empty())
    init_table();
}

#ifdef COMP_MSC
#pragma warning(disable:4146)	//unary minus operator applied to unsigned type, result still unsigned
#endif

void
crc_base_c::init_table() {
  auto &parameters = ms_table_parameters[m_type];

  if ((parameters.bits < 8) || (parameters.bits > 32) || (parameters.poly >= (1LL<<parameters.bits)))
    throw std::domain_error{"Invalid CRC parameters"};

  m_table.resize(256);

  for (auto i = 0u; i < 256u; i++) {
    if (parameters.le) {
      uint32_t c = i;
      for (auto j = 0u; j < 8u; j++)
        c = (c >> 1) ^ (parameters.poly & (-(c & 1)));
      m_table[i] = c;

    } else {
      uint32_t c = i << 24;
      for (auto j = 0u; j < 8u; j++)
        c = (c << 1) ^ ((parameters.poly << (32 - parameters.bits)) & (static_cast<int32_t>(c) >> 31));
      m_table[i] = mtx::bytes::swap_32(c);
    }
  }

  // for (auto row = 0u; row < (256u / 4); ++row)
  //   mxinfo(fmt::format("0x{0:08x} 0x{1:08x} 0x{2:08x} 0x{3:08x}\n", m_table[row * 4 + 0], m_table[row * 4 + 1], m_table[row * 4 + 2], m_table[row * 4 + 3]));
}

memory_cptr
crc_base_c::get_result()
  const {
  auto result_length = ms_table_parameters[m_type].bits / 8;
  auto result        = m_crc ^ m_xor_result;

  if (m_result_in_le)
    result = result_length == 4 ? mtx::bytes::swap_32(result)
           : result_length == 2 ? mtx::bytes::swap_16(result)
           :                      result;

  auto buffer = memory_c::alloc(result_length);

  put_uint_be(buffer->get_buffer(), result, result_length);

  return buffer;
}

uint64_t
crc_base_c::get_result_as_uint()
  const {
  return m_crc ^ m_xor_result;
}

void
crc_base_c::set_initial_value_impl(uint64_t initial_value) {
  m_crc = initial_value;
}

void
crc_base_c::set_initial_value_impl(uint8_t const *buffer,
                                   size_t size) {
  m_crc = get_uint_be(buffer, size);
}

void
crc_base_c::set_xor_result(uint64_t xor_result) {
  m_xor_result = xor_result;
}

void
crc_base_c::set_result_in_le(bool result_in_le) {
  m_result_in_le = result_in_le;
}

void
crc_base_c::add_impl(uint8_t const *buffer,
                     size_t size) {
  auto end = buffer + size;

  while (buffer < end) {
    m_crc = m_table[(m_crc & 0xff) ^ *buffer] ^ (m_crc >> 8);
    ++buffer;
  }
}

// ----------------------------------------------------------------------

crc_base_c::table_t crc8_atm_c::ms_table;

crc8_atm_c::crc8_atm_c(uint32_t initial_value)
  : crc_base_c{crc_8_atm, ms_table, initial_value}
{
}

// ----------------------------------------------------------------------

crc_base_c::table_t crc16_ansi_c::ms_table;

crc16_ansi_c::crc16_ansi_c(uint32_t initial_value)
  : crc_base_c{crc_16_ansi, ms_table, initial_value}
{
}

// ----------------------------------------------------------------------

crc_base_c::table_t crc16_ccitt_c::ms_table;

crc16_ccitt_c::crc16_ccitt_c(uint32_t initial_value)
  : crc_base_c{crc_16_ccitt, ms_table, initial_value}
{
}

// ----------------------------------------------------------------------

crc_base_c::table_t crc16_002d_c::ms_table;

crc16_002d_c::crc16_002d_c(uint32_t initial_value)
  : crc_base_c{crc_16_002d, ms_table, initial_value}
{
}

// ----------------------------------------------------------------------

crc_base_c::table_t crc32_ieee_c::ms_table;

crc32_ieee_c::crc32_ieee_c(uint32_t initial_value)
  : crc_base_c{crc_32_ieee, ms_table, initial_value}
{
}

// ----------------------------------------------------------------------

crc_base_c::table_t crc32_ieee_le_c::ms_table;

crc32_ieee_le_c::crc32_ieee_le_c(uint32_t initial_value)
  : crc_base_c{crc_32_ieee_le, ms_table, initial_value}
{
}

} // namespace mtx::checksum
