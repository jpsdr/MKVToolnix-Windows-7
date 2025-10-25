/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for a memory slice cursor

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx::mem {

class slice_cursor_c {
protected:
  std::size_t m_pos{}, m_pos_in_slice{}, m_size{}, m_current_slice_num{}, m_current_slice_size{};
  uint8_t *m_current_slice_buffer{};
  std::vector<memory_cptr> m_slices;

public:
  slice_cursor_c()
  {
  }

  ~slice_cursor_c() {
  }

  slice_cursor_c(slice_cursor_c const &) = delete;

  void init_slice_variables() {
    if (m_current_slice_num < m_slices.size()) {
      auto &slice            = *m_slices[m_current_slice_num];
      m_current_slice_buffer = slice.get_buffer();
      m_current_slice_size   = slice.get_size();
    } else {
      m_current_slice_buffer = nullptr;
      m_current_slice_size   = 0;
    }
  }

  void add_slice(memory_cptr const &slice) {
    if (slice->get_size() == 0)
      return;

    m_slices.emplace_back(slice);
    m_size += slice->get_size();

    if (!m_current_slice_buffer)
      init_slice_variables();
  }

  void add_slice(uint8_t *buffer, std::size_t size) {
    if (0 == size)
      return;

    add_slice(memory_c::borrow(buffer, size));
  }

  inline uint8_t get_char() {
    assert(m_current_slice_buffer && (m_pos < m_size));

    auto c = m_current_slice_buffer[m_pos_in_slice];

    ++m_pos_in_slice;
    ++m_pos;

    if (m_pos_in_slice < m_current_slice_size)
      return c;

    ++m_current_slice_num;
    m_pos_in_slice = 0;

    init_slice_variables();

    return c;
  }

  inline bool char_available() {
    return m_pos < m_size;
  }

  inline std::size_t get_remaining_size() {
    return m_size - m_pos;
  }

  inline std::size_t get_size() {
    return m_size;
  }

  inline std::size_t get_position() {
    return m_pos;
  }

  void reset(bool clear_slices = false) {
    if (clear_slices) {
      m_slices.clear();
      m_size = 0;
    }

    m_pos               = 0;
    m_pos_in_slice      = 0;
    m_current_slice_num = 0;

    init_slice_variables();
  }

  void copy(uint8_t *dest, std::size_t start, std::size_t size) {
    assert((start + size) <= m_size);

    auto curr   = m_slices.begin();
    auto offset = 0u;

    while (start > ((*curr)->get_size() + offset)) {
      offset += (*curr)->get_size();
      ++curr;
      assert(m_slices.end() != curr);
    }
    offset = start - offset;

    while (0 < size) {
      auto num_bytes = (*curr)->get_size() - offset;
      if (num_bytes > size)
        num_bytes = size;

      std::memcpy(dest, (*curr)->get_buffer() + offset, num_bytes);

      size   -= num_bytes;
      dest   += num_bytes;
      offset  = 0;
      ++curr;
    }
  }
};

}
