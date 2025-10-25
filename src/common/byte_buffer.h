/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Byte buffer class

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/memory.h"

namespace mtx::bytes {

class buffer_c {
private:
  memory_cptr m_data;
  std::size_t m_filled, m_offset, m_size, m_chunk_size;
  std::size_t m_num_reallocs, m_max_alloced_size;

public:
  enum position_e {
      at_front
    , at_back
  };

  buffer_c(std::size_t chunk_size = 128 * 1024)
    : m_data{memory_c::alloc(chunk_size)}
    , m_filled{}
    , m_offset{}
    , m_size{chunk_size}
    , m_chunk_size{chunk_size}
    , m_num_reallocs{1}
    , m_max_alloced_size{chunk_size}
  {
  };

  void trim() {
    if (m_offset == 0)
      return;

    auto buffer = m_data->get_buffer();
    std::memmove(buffer, &buffer[m_offset], m_filled);

    m_offset             = 0;
    std::size_t new_size = (m_filled / m_chunk_size + 1) * m_chunk_size;

    if (new_size != m_size) {
      m_data->resize(new_size);
      m_size = new_size;

      count_alloc(new_size);
    }
  }

  void add(uint8_t const *new_data, std::size_t new_size, position_e const add_where = at_back) {
    if ((m_offset != 0) && ((m_offset + m_filled + new_size) >= m_chunk_size))
      trim();

    if ((m_offset + m_filled + new_size) > m_size) {
      m_size = ((m_offset + m_filled + new_size) / m_chunk_size + 1) * m_chunk_size;
      m_data->resize(m_size);
      count_alloc(m_size);
    }

    if (add_where == at_back)
      std::memcpy(m_data->get_buffer() + m_offset + m_filled, new_data, new_size);
    else {
      auto target = m_data->get_buffer() + m_offset;
      std::memmove(target + new_size, target, m_filled);
      std::memcpy(target, new_data, new_size);
    }

    m_filled += new_size;
  }

  void add(memory_c &new_buffer, position_e const add_where = at_back) {
    add(new_buffer.get_buffer(), new_buffer.get_size(), add_where);
  }

  void prepend(uint8_t const *new_data, std::size_t new_size) {
    add(new_data, new_size, at_front);
  }

  void prepend(memory_c &new_buffer) {
    add(new_buffer.get_buffer(), new_buffer.get_size(), at_front);
  }

  void remove(std::size_t num, position_e const remove_where = at_front) {
    if (num > m_filled)
      mxerror("buffer_c: num > m_filled. Should not have happened. Please file a bug report.\n");

    if (remove_where == at_front)
      m_offset += num;
    m_filled -= num;

    if (m_filled >= m_chunk_size)
      trim();
  }

  void clear() {
    if (m_filled)
      remove(m_filled);
  }

  uint8_t *get_buffer() const {
    return m_data->get_buffer() + m_offset;
  }

  std::size_t get_size() const {
    return m_filled;
  }

  void set_chunk_size(size_t chunk_size) {
    m_chunk_size = chunk_size;
    trim();
  }

private:

  void count_alloc(size_t filled) {
    ++m_num_reallocs;
    m_max_alloced_size = std::max(m_max_alloced_size, filled);
  }
};

using buffer_cptr = std::shared_ptr<buffer_c>;

}
