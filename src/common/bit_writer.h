/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   A class for file-like write access on the bit level
*/

#pragma once

#include "common/common_pch.h"

#include "common/bit_reader.h"

namespace mtx::bits {

class writer_c {
private:
  memory_cptr m_buffer;
  unsigned char *m_data{};
  std::size_t m_size{}, m_byte_position{}, m_mask{0x80u}, m_data_size{};
  bool m_can_extend{};

public:
  writer_c()
    : m_buffer{memory_c::alloc(100)}
    , m_data{m_buffer->get_buffer()}
    , m_data_size{100}
    , m_can_extend{true}
  {
    std::memset(m_buffer->get_buffer(), 0, m_buffer->get_size());
  }

  writer_c(unsigned char *data, std::size_t size)
    : m_data{data}
    , m_size{size}
    , m_data_size{size}
  {
  }

  writer_c(memory_c &data)
    : m_data{data.get_buffer()}
    , m_size{data.get_size()}
    , m_data_size{data.get_size()}
  {
  }

  inline memory_cptr get_buffer() {
    return memory_c::clone(m_data, m_size);
  }

  inline uint64_t copy_bits(std::size_t n, reader_c &src) {
    uint64_t value{};

    while (n) {
      auto to_copy = std::min<std::size_t>(n, 64);
      n           -= to_copy;
      value        = src.get_bits(to_copy);
      put_bits(to_copy, value);
    }

    return value;
  }

  inline uint64_t copy_unsigned_golomb(reader_c &r) {
    int n = 0;

    while (r.get_bit() == 0) {
      put_bit(false);
      ++n;
    }

    put_bit(true);

    auto bits = copy_bits(n, r);

    return (1 << n) - 1 + bits;
  }

  inline int64_t copy_signed_golomb(reader_c &r) {
    int64_t v = copy_unsigned_golomb(r);
    return v & 1 ? (v + 1) / 2 : -(v / 2);
  }

  inline void put_bits(std::size_t n, uint64_t value) {
    while (0 < n) {
      put_bit((value & (1ull << (n - 1))) != 0);
      --n;
    }
  }

  inline void put_bit(bool bit) {
    extend_buffer_if_needed();

    if (bit)
      m_data[m_byte_position] |=  m_mask;
    else
      m_data[m_byte_position] &= ~m_mask;

    m_mask >>= 1;

    if (0 == m_mask) {
      m_mask = 0x80;
      ++m_byte_position;
    }

    update_size();
  }

  inline void put_leb128(uint64_t value) {
    do {
      auto this_byte = value & 0x7f;
      value        >>= 7;

      if (value != 0)
        this_byte |= 0x80;

      put_bits(8, this_byte);

    } while (value != 0);
  }

  inline void byte_align() {
    while (0x80 != m_mask)
      put_bit(false);
  }

  inline void set_bit_position(std::size_t pos) {
    m_byte_position = pos / 8;
    m_mask          = 0x80 >> (pos % 8);
  }

  inline std::size_t get_bit_position() const {
    std::size_t pos = m_byte_position * 8;
    for (auto i = 0u; 8 > i; ++i)
      if ((0x80u >> i) == m_mask) {
        pos += i;
        break;
      }
    return pos;
  }

  inline void skip_bits(unsigned int num) {
    set_bit_position(get_bit_position() + num);
  }

  inline void skip_bit() {
    set_bit_position(get_bit_position() + 1);
  }

protected:
  inline void extend_buffer_if_needed() {
    if (m_byte_position < m_data_size)
      return;

    if (!m_can_extend)
      throw std::invalid_argument{"bit_writer_c: cannot extend provided buffer"};

    m_data_size = (m_byte_position / 100 + 1) * 100;
    m_buffer->resize(m_data_size);
    m_data = m_buffer->get_buffer();

    std::memset(&m_data[m_size], 0, m_buffer->get_size() - m_size);
  }

  inline void update_size() {
    m_size = std::max(m_size, m_byte_position + (m_mask == 0x80 ? 0 : 1));
  }
};
using writer_cptr = std::shared_ptr<writer_c>;

}
