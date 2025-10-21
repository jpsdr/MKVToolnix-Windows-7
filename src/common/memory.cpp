/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for memory handling classes

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/memory.h"
#include "common/error.h"

void
memory_c::resize(size_t new_size)
  noexcept
{
  if (new_size == m_size)
    return;

  if (m_is_owned) {
    m_ptr  = static_cast<uint8_t *>(saferealloc(m_ptr, new_size + m_offset));
    m_size = new_size + m_offset;

  } else {
    auto tmp = static_cast<uint8_t *>(safemalloc(new_size));
    std::memcpy(tmp, m_ptr + m_offset, std::min(new_size, m_size - m_offset));
    m_ptr      = tmp;
    m_is_owned = true;
    m_size     = new_size;
  }
}

void
memory_c::add(uint8_t const *new_buffer,
              size_t new_size) {
  if ((0 == new_size) || !new_buffer)
    return;

  auto previous_size = get_size();
  resize(previous_size + new_size);
  std::memcpy(get_buffer() + previous_size, new_buffer, new_size);
}

void
memory_c::prepend(uint8_t const *new_buffer,
                  size_t new_size) {
  if ((0 == new_size) || !new_buffer)
    return;

  auto previous_size = get_size();
  resize(previous_size + new_size);

  auto buffer = get_buffer();

  std::memmove(&buffer[new_size], &buffer[0],     previous_size);
  std::memcpy( &buffer[0],        &new_buffer[0], new_size);
}

memory_cptr
lace_memory_xiph(std::vector<memory_cptr> const &blocks) {
  size_t i, size = 1;
  for (i = 0; (blocks.size() - 1) > i; ++i)
    size += blocks[i]->get_size() / 255 + 1 + blocks[i]->get_size();
  size += blocks.back()->get_size();

  memory_cptr mem       = memory_c::alloc(size);
  uint8_t *buffer = mem->get_buffer();

  buffer[0]             = blocks.size() - 1;
  size_t offset         = 1;
  for (i = 0; (blocks.size() - 1) > i; ++i) {
    int n;
    for (n = blocks[i]->get_size(); n >= 255; n -= 255) {
      buffer[offset] = 255;
      ++offset;
    }
    buffer[offset] = n;
    ++offset;
  }

  for (i = 0; blocks.size() > i; ++i) {
    memcpy(&buffer[offset], blocks[i]->get_buffer(), blocks[i]->get_size());
    offset += blocks[i]->get_size();
  }

  return mem;
}

std::vector<memory_cptr>
unlace_memory_xiph(memory_cptr const &buffer) {
  if (1 > buffer->get_size())
    throw mtx::mem::lacing_x("Buffer too small");

  std::vector<int> sizes;
  auto ptr          = buffer->get_buffer();
  auto end          = buffer->get_buffer() + buffer->get_size();
  auto last_size    = buffer->get_size();
  size_t num_blocks = ptr[0] + 1;
  size_t i;
  ++ptr;

  for (i = 0; (num_blocks - 1) > i; ++i) {
    int size = 0;
    while ((ptr < end) && (*ptr == 255)) {
      size += 255;
      ++ptr;
    }

    if (ptr >= end)
      throw mtx::mem::lacing_x("End-of-buffer while reading the block sizes");

    size += *ptr;
    ++ptr;

    sizes.push_back(size);
    last_size -= size;
  }

  sizes.push_back(last_size - (ptr - buffer->get_buffer()));

  std::vector<memory_cptr> blocks;

  for (i = 0; sizes.size() > i; ++i) {
    if ((ptr + sizes[i]) > end)
      throw mtx::mem::lacing_x("End-of-buffer while assigning the blocks");

    blocks.push_back(memory_c::borrow(ptr, sizes[i]));
    ptr += sizes[i];
  }

  return blocks;
}

uint8_t *
_safememdup(const void *s,
            size_t size,
            const char *file,
            int line) {
  if (!s)
    return nullptr;

  auto copy = reinterpret_cast<uint8_t *>(malloc(size));
  if (!copy)
    mxerror(fmt::format(FY("memory.cpp/safememdup() called from file {0}, line {1}: malloc() returned nullptr for a size of {2} bytes.\n"), file, line, size));
  memcpy(copy, s, size);        // NOLINT(clang-analyzer-core.NonNullParamChecker) as mxerror() terminates the program

  return copy;
}

uint8_t *
_safemalloc(size_t size,
            const char *file,
            int line) {
  auto mem = reinterpret_cast<uint8_t *>(malloc(size));
  if (!mem)
    mxerror(fmt::format(FY("memory.cpp/safemalloc() called from file {0}, line {1}: malloc() returned nullptr for a size of {2} bytes.\n"), file, line, size));

  return mem;
}

uint8_t *
_saferealloc(void *mem,
             size_t size,
             const char *file,
             int line) {
  if (0 == size)
    // Do this so realloc() may not return nullptr on success.
    size = 1;

  mem = realloc(mem, size);
  if (!mem)
    mxerror(fmt::format(FY("memory.cpp/saferealloc() called from file {0}, line {1}: realloc() returned nullptr for a size of {2} bytes.\n"), file, line, size));

  return reinterpret_cast<uint8_t *>(mem);
}

// ----------------------------------------------------------------------

memory_c &
memory_c::splice(memory_c &buffer,
                 std::size_t offset,
                 std::size_t to_remove,
                 std::optional<std::reference_wrapper<memory_c>> to_insert) {
  auto size = buffer.get_size();

  if ((offset + to_remove) > size)
    throw std::invalid_argument{fmt::format("splice: (offset + to_remove) > buffer_size: ({0} + {1}) >= {2}", offset, to_remove, size)};

  auto insert_size = to_insert ? to_insert.value().get().get_size() : 0;
  auto diff        = static_cast<int64_t>(to_remove) - static_cast<int64_t>(insert_size);
  auto remaining   = size - offset - to_remove;

  if (diff < 0)
    buffer.resize(size - diff);

  if ((remaining > 0) && (diff != 0))
    std::memmove(buffer.get_buffer() + offset + insert_size, buffer.get_buffer() + offset + to_remove, remaining);

  if (to_insert)
    std::memcpy(buffer.get_buffer() + offset, to_insert.value().get().get_buffer(), insert_size);

  buffer.resize(size - diff);

  return buffer;
}
