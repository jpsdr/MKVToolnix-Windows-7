/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definition for a memory slice cursor

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include <deque>

namespace mtx { namespace mem {

class slice_cursor_c {
protected:
  std::size_t m_pos{}, m_pos_in_slice{}, m_size{};
  std::deque<memory_cptr> m_slices;
  std::deque<memory_cptr>::iterator m_slice;

public:
  slice_cursor_c()
    : m_slice(m_slices.end())
  {
  }

  ~slice_cursor_c() {
  }

  slice_cursor_c(slice_cursor_c const &) = delete;

  void add_slice(memory_cptr const &slice) {
    if (slice->get_size() == 0)
      return;

    if (m_slices.end() == m_slice) {
      m_slices.push_back(slice);
      m_slice = m_slices.begin();

    } else {
      auto pos = std::distance(m_slices.begin(), m_slice);
      m_slices.push_back(slice);
      m_slice = m_slices.begin() + pos;
    }

    m_size += slice->get_size();
  }

  void add_slice(unsigned char *buffer, std::size_t size) {
    if (0 == size)
      return;

    add_slice(std::make_shared<memory_c>(buffer, size, false));
  }

  inline unsigned char get_char() {
    assert(m_pos < m_size);

    auto &slice = *m_slice->get();
    auto c      = *(slice.get_buffer() + m_pos_in_slice);

    ++m_pos_in_slice;
    ++m_pos;

    if (m_pos_in_slice < slice.get_size())
      return c;

    ++m_slice;
    m_pos_in_slice = 0;

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
    m_pos          = 0;
    m_pos_in_slice = 0;
    m_slice        = m_slices.begin();
  }

  void copy(unsigned char *dest, std::size_t start, std::size_t size) {
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

}}
