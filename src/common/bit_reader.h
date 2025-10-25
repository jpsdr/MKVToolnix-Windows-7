/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   A class for file-like read access on the bit level

   The mtx::bits::reader_c class was originally written by Peter Niemayer
     <niemayer@isg.de> and modified by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/mm_io_x.h"

namespace mtx::bits {

class reader_c {
private:
  const uint8_t *m_end_of_data;
  const uint8_t *m_byte_position;
  const uint8_t *m_start_of_data;
  std::size_t m_bits_valid;
  bool m_out_of_data, m_rbsp_mode;
  uint16_t m_rbsp_bytes;

public:
  reader_c() {
    init(nullptr, 0);
  }

  reader_c(uint8_t const *data, std::size_t len) {
    init(data, len);
  }

  reader_c(memory_c &data) {
    init(data.get_buffer(), data.get_size());
  }

  void init(const uint8_t *data, std::size_t len) {
    m_end_of_data   = data + len;
    m_byte_position = data;
    m_start_of_data = data;
    m_bits_valid    = len ? 8 : 0;
    m_out_of_data   = m_byte_position >= m_end_of_data;
    m_rbsp_mode     = false;
    m_rbsp_bytes    = 0xffffu;
  }

  void enable_rbsp_mode() {
    m_rbsp_mode  = true;
    m_rbsp_bytes = 0xff00u | (m_bits_valid == 8 ? *m_byte_position : 0u);
  }

  bool eof() {
    return m_out_of_data;
  }

  uint64_t get_bits(std::size_t n) {
    uint64_t r = 0;

    while (n > 0) {
      if (m_byte_position >= m_end_of_data) {
        m_out_of_data = true;
        throw mtx::mm_io::end_of_file_x();
      }

      std::size_t b = 8; // number of bits to extract from the current byte
      if (b > n)
        b = n;
      if (b > m_bits_valid)
        b = m_bits_valid;

      std::size_t rshift = m_bits_valid - b;

      r <<= b;
      r  |= ((*m_byte_position) >> rshift) & (0xff >> (8 - b));

      m_bits_valid -= b;
      if (0 == m_bits_valid) {
        m_bits_valid     = 8;
        m_byte_position += 1;

        if (m_rbsp_mode && (m_byte_position < m_end_of_data)) {
          if ((*m_byte_position == 0x03) && (m_rbsp_bytes == 0x0000)) {
            ++m_byte_position;
            m_rbsp_bytes = 0xff00u | *m_byte_position;

          } else
            m_rbsp_bytes = (m_rbsp_bytes << 8) | *m_byte_position;
        }
      }

      n -= b;
    }

    return r;
  }

  inline int get_bit() {
    return get_bits(1);
  }

  inline int get_unary(bool stop,
                       int len) {
    int i;

    for (i = 0; (i < len) && get_bit() != stop; ++i)
      ;

    return i;
  }

  inline int get_012() {
    if (!get_bit())
      return 0;
    return get_bits(1) + 1;
  }

  inline uint64_t get_unsigned_golomb() {
    int n = 0;

    while (get_bit() == 0)
      ++n;

    auto bits = get_bits(n);

    return (1u << n) - 1 + bits;
  }

  inline int64_t get_signed_golomb() {
    int64_t v = get_unsigned_golomb();
    return v & 1 ? (v + 1) / 2 : -(v / 2);
  }

  inline uint64_t get_leb128() {
    uint64_t value{};

    for (int idx = 0; idx < 8; ++idx) {
      auto byte  = get_bits(8);
      value     |= (byte & 0x7f) << (idx * 7);

      if ((byte & 0x80) == 0)
        break;
    }

    return value;
  }

  uint64_t peek_bits(std::size_t n) {
    uint64_t r                       = 0;
    const uint8_t *tmp_byte_position = m_byte_position;
    std::size_t tmp_bits_valid       = m_bits_valid;

    while (0 < n) {
      if (tmp_byte_position >= m_end_of_data)
        throw mtx::mm_io::end_of_file_x();

      std::size_t b = 8; // number of bits to extract from the current byte
      if (b > n)
        b = n;
      if (b > tmp_bits_valid)
        b = tmp_bits_valid;

      std::size_t rshift = tmp_bits_valid - b;

      r <<= b;
      r  |= ((*tmp_byte_position) >> rshift) & (0xff >> (8 - b));

      tmp_bits_valid -= b;
      if (0 == tmp_bits_valid) {
        tmp_bits_valid     = 8;
        tmp_byte_position += 1;
      }

      n -= b;
    }

    return r;
  }

  void get_bytes(uint8_t *buf, std::size_t n) {
    if (8 == m_bits_valid) {
      get_bytes_byte_aligned(buf, n);
      return;
    }

    for (auto idx = 0u; idx < n; ++idx)
      buf[idx] = get_bits(8);
  }

  std::string get_string(unsigned int byte_length) {
    std::string str(byte_length, ' ');
    for (auto idx = 0u; idx < byte_length; ++idx)
      str[idx] = static_cast<char>(get_bits(8));

    return str;
  }

  void byte_align() {
    if (8 != m_bits_valid)
      skip_bits(m_bits_valid);
  }

  void set_bit_position(std::size_t pos) {
    if (pos > (static_cast<std::size_t>(m_end_of_data - m_start_of_data) * 8)) {
      m_byte_position = m_end_of_data;
      m_out_of_data   = true;

      throw mtx::mm_io::end_of_file_x();
    }

    m_byte_position = m_start_of_data + (pos / 8);
    m_bits_valid    = 8 - (pos % 8);
  }

  int get_bit_position() const {
    return (m_byte_position - m_start_of_data) * 8 + (m_bits_valid ? 8 - m_bits_valid : 0);
  }

  int get_remaining_bits() const {
    return (m_end_of_data - m_byte_position) * 8 - (m_bits_valid ? 8 - m_bits_valid : 0);
  }

  void skip_bits(std::size_t num) {
    if (!m_rbsp_mode)
      set_bit_position(get_bit_position() + num);
    else
      get_bits(num);
  }

  void skip_bit() {
    if (!m_rbsp_mode)
      set_bit_position(get_bit_position() + 1);
    else
      get_bits(1);
  }

  uint64_t skip_get_bits(std::size_t to_skip,
                         std::size_t to_get) {
    skip_bits(to_skip);
    return get_bits(to_get);
  }

protected:
  void get_bytes_byte_aligned(uint8_t *buf, std::size_t n) {
    auto bytes_to_copy = std::min<std::size_t>(n, m_end_of_data - m_byte_position);
    std::memcpy(buf, m_byte_position, bytes_to_copy);

    m_byte_position += bytes_to_copy;

    if (bytes_to_copy < n) {
      m_out_of_data = true;
      throw mtx::mm_io::end_of_file_x();
    }
  }
};
using reader_cptr = std::shared_ptr<reader_c>;

}
