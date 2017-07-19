/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   A class for file-like write access on the bit level
*/

#ifndef MTX_COMMON_BIT_WRITER_H
#define MTX_COMMON_BIT_WRITER_H

#include "common/common_pch.h"

#include "common/bit_reader.h"

class bit_writer_c {
private:
  unsigned char *m_end_of_data;
  unsigned char *m_byte_position;
  unsigned char *m_start_of_data;
  std::size_t m_mask;

  bool m_out_of_data;

public:
  bit_writer_c(unsigned char *data, std::size_t len)
    : m_end_of_data(data + len)
    , m_byte_position(data)
    , m_start_of_data(data)
    , m_mask(0x80)
    , m_out_of_data(m_byte_position >= m_end_of_data)
  {
  }

  uint64_t copy_bits(std::size_t n, bit_reader_c &src) {
    uint64_t value = src.get_bits(n);
    put_bits(n, value);

    return value;
  }

  inline uint64_t copy_unsigned_golomb(bit_reader_c &r) {
    int n = 0;

    while (r.get_bit() == 0) {
      put_bit(0);
      ++n;
    }

    put_bit(1);

    auto bits = copy_bits(n, r);

    return (1 << n) - 1 + bits;
  }

  inline int64_t copy_signed_golomb(bit_reader_c &r) {
    int64_t v = copy_unsigned_golomb(r);
    return v & 1 ? (v + 1) / 2 : -(v / 2);
  }

  void put_bits(std::size_t n, uint64_t value) {
    while (0 < n) {
      put_bit(value & (1 << (n - 1)));
      --n;
    }
  }

  void put_bit(bool bit) {
    if (m_byte_position >= m_end_of_data) {
      m_out_of_data = true;
      throw mtx::mm_io::end_of_file_x();
    }

    if (bit)
      *m_byte_position |=  m_mask;
    else
      *m_byte_position &= ~m_mask;
    m_mask >>= 1;
    if (0 == m_mask) {
      m_mask = 0x80;
      ++m_byte_position;
      if (m_byte_position == m_end_of_data)
        m_out_of_data = true;
    }
  }

  void byte_align() {
    while (0x80 != m_mask)
      put_bit(0);
  }

  void set_bit_position(std::size_t pos) {
    if (pos >= (static_cast<std::size_t>(m_end_of_data - m_start_of_data) * 8)) {
      m_byte_position = m_end_of_data;
      m_out_of_data   = true;

      throw mtx::mm_io::seek_x();
    }

    m_byte_position = m_start_of_data + (pos / 8);
    m_mask          = 0x80 >> (pos % 8);
  }

  int get_bit_position() {
    std::size_t pos = (m_byte_position - m_start_of_data) * 8;
    for (auto i = 0u; 8 > i; ++i)
      if ((0x80u >> i) == m_mask) {
        pos += i;
        break;
      }
    return pos;
  }

  void skip_bits(unsigned int num) {
    set_bit_position(get_bit_position() + num);
  }

  void skip_bit() {
    set_bit_position(get_bit_position() + 1);
  }
};
using bit_writer_cptr = std::shared_ptr<bit_writer_c>;

#endif // MTX_COMMON_BIT_WRITER_H
