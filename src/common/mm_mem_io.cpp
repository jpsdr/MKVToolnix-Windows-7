 /*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   IO callback class implementation

   Written by Moritz Bunkus <moritz@bunkus.org>.
   Modifications by Peter Niemayer <niemayer@isg.de>.
*/

#include "common/common_pch.h"

#include "common/mm_io_x.h"
#include "common/mm_mem_io.h"

/*
   IO callback class working on memory
*/
mm_mem_io_c::mm_mem_io_c(unsigned char *mem,
                         uint64_t mem_size,
                         int increase)
  : m_pos(0)
  , m_mem_size(mem_size)
  , m_allocated(mem_size)
  , m_increase(increase)
  , m_mem(mem)
  , m_ro_mem(nullptr)
  , m_read_only(false)
{
  if (0 >= m_increase)
    throw mtx::invalid_parameter_x();

  if (!m_mem && (0 < m_increase)) {
    if (0 == mem_size)
      m_allocated = increase;

    m_mem      = safemalloc(m_allocated);
    m_free_mem = true;

  } else
    m_free_mem = false;
}

mm_mem_io_c::mm_mem_io_c(const unsigned char *mem,
                         uint64_t mem_size)
  : m_pos(0)
  , m_mem_size(mem_size)
  , m_allocated(mem_size)
  , m_increase(0)
  , m_mem(nullptr)
  , m_ro_mem(mem)
  , m_free_mem(false)
  , m_read_only(true)
{
  if (!m_ro_mem)
    throw mtx::invalid_parameter_x();
}

mm_mem_io_c::mm_mem_io_c(memory_c const &mem)
  : m_pos{}
  , m_mem_size{mem.get_size()}
  , m_allocated{mem.get_size()}
  , m_increase{}
  , m_mem{}
  , m_ro_mem{mem.get_buffer()}
  , m_free_mem{}
  , m_read_only{true}
{
  if (!m_ro_mem)
    throw mtx::invalid_parameter_x{};
}

mm_mem_io_c::~mm_mem_io_c() {
  close();
}

uint64
mm_mem_io_c::getFilePointer() {
  return m_pos;
}

void
mm_mem_io_c::setFilePointer(int64 offset,
                            libebml::seek_mode mode) {
  if (!m_mem && !m_ro_mem && (0 == m_mem_size))
    throw mtx::invalid_parameter_x();

  int64_t new_pos
    = libebml::seek_beginning == mode ? offset
    : libebml::seek_end       == mode ? m_mem_size + offset // offsets from the end are negative already
    :                                   m_pos      + offset;

  if ((0 <= new_pos) && (static_cast<int64_t>(m_mem_size) >= new_pos))
    m_pos = new_pos;
  else
    throw mtx::mm_io::seek_x{mtx::mm_io::make_error_code()};
}

uint32
mm_mem_io_c::_read(void *buffer,
                   size_t size) {
  size_t rbytes = std::min(size, m_mem_size - m_pos);
  if (m_read_only)
    memcpy(buffer, &m_ro_mem[m_pos], rbytes);
  else
    memcpy(buffer, &m_mem[m_pos], rbytes);
  m_pos += rbytes;

  return rbytes;
}

size_t
mm_mem_io_c::_write(const void *buffer,
                    size_t size) {
  if (m_read_only)
    throw mtx::mm_io::wrong_read_write_access_x();

  int64_t wbytes;
  if ((m_pos + size) >= m_allocated) {
    if (m_increase) {
      int64_t new_allocated  = m_pos + size - m_allocated;
      new_allocated          = ((new_allocated / m_increase) + 1 ) * m_increase;
      m_allocated           += new_allocated;
      m_mem                  = (unsigned char *)saferealloc(m_mem, m_allocated);
      wbytes                 = size;
    } else
      wbytes                 = m_allocated - m_pos;

  } else
    wbytes = size;

  if ((m_pos + size) > m_mem_size)
    m_mem_size = m_pos + size;

  memcpy(&m_mem[m_pos], buffer, wbytes);
  m_pos         += wbytes;
  m_cached_size  = -1;

  return wbytes;
}

void
mm_mem_io_c::close() {
  if (m_free_mem)
    safefree(m_mem);
  m_mem       = nullptr;
  m_ro_mem    = nullptr;
  m_read_only = true;
  m_free_mem  = false;
  m_mem_size  = 0;
  m_increase  = 0;
  m_pos       = 0;
}

bool
mm_mem_io_c::eof() {
  return m_pos >= m_mem_size;
}

unsigned char *
mm_mem_io_c::get_buffer()
  const {
  if (m_read_only)
    throw mtx::invalid_parameter_x();
  return m_mem;
}

unsigned char const *
mm_mem_io_c::get_ro_buffer()
  const {
  if (!m_read_only)
    throw mtx::invalid_parameter_x();
  return m_ro_mem;
}

memory_cptr
mm_mem_io_c::get_and_lock_buffer() {
  m_free_mem = false;
  return memory_c::take_ownership(m_mem, getFilePointer());
}

std::string
mm_mem_io_c::get_content()
  const {
  char const *source = m_read_only ? reinterpret_cast<char const *>(m_ro_mem) : reinterpret_cast<char const *>(m_mem);

  if (!source || !m_mem_size)
    return std::string{};

  return std::string(source, m_mem_size);
}
