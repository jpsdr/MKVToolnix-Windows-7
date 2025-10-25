 /*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   IO callback class implementation

   Written by Moritz Bunkus <mo@bunkus.online>.
   Modifications by Peter Niemayer <niemayer@isg.de>.
*/

#include "common/common_pch.h"

#include "common/mm_io_x.h"
#include "common/mm_mem_io.h"
#include "common/mm_mem_io_p.h"

/*
   IO callback class working on memory
*/
mm_mem_io_c::mm_mem_io_c(uint8_t *mem,
                         uint64_t mem_size,
                         std::size_t increase)
  : mm_io_c{*new mm_mem_io_private_c{mem, mem_size, increase}}
{
}

mm_mem_io_c::mm_mem_io_c(const uint8_t *mem,
                         uint64_t mem_size)
  : mm_io_c{*new mm_mem_io_private_c{mem, mem_size}}
{
}

mm_mem_io_c::mm_mem_io_c(memory_c const &mem)
  : mm_io_c{*new mm_mem_io_private_c{mem}}
{
}

mm_mem_io_c::mm_mem_io_c(mm_mem_io_private_c &p)
  : mm_io_c{p}
{
}

mm_mem_io_c::~mm_mem_io_c() {
  close_mem_io();
}

uint64_t
mm_mem_io_c::getFilePointer() {
  return p_func()->pos;
}

void
mm_mem_io_c::setFilePointer(int64_t offset,
                            libebml::seek_mode mode) {
  auto p = p_func();

  if (!p->mem && !p->ro_mem && (0 == p->mem_size))
    throw mtx::invalid_parameter_x();

  int64_t new_pos
    = libebml::seek_beginning == mode ? offset
    : libebml::seek_end       == mode ? p->mem_size + offset // offsets from the end are negative already
    :                                   p->pos      + offset;

  if ((0 <= new_pos) && (static_cast<int64_t>(p->mem_size) >= new_pos))
    p->pos = new_pos;
  else
    throw mtx::mm_io::seek_x{mtx::mm_io::make_error_code()};
}

uint32_t
mm_mem_io_c::_read(void *buffer,
                   size_t size) {
  auto p        = p_func();
  size_t rbytes = std::min(size, p->mem_size - p->pos);
  if (p->read_only)
    memcpy(buffer, &p->ro_mem[p->pos], rbytes);
  else
    memcpy(buffer, &p->mem[p->pos], rbytes);
  p->pos += rbytes;

  return rbytes;
}

size_t
mm_mem_io_c::_write(const void *buffer,
                    size_t size) {
  auto p = p_func();

  if (p->read_only)
    throw mtx::mm_io::wrong_read_write_access_x();

  int64_t wbytes;
  if ((p->pos + size) >= p->allocated) {
    if (p->increase) {
      int64_t new_allocated  = p->pos + size - p->allocated;
      new_allocated          = ((new_allocated / p->increase) + 1 ) * p->increase;
      p->allocated           += new_allocated;
      p->mem                  = (uint8_t *)saferealloc(p->mem, p->allocated);
      wbytes                 = size;
    } else
      wbytes                 = p->allocated - p->pos;

  } else
    wbytes = size;

  if ((p->pos + size) > p->mem_size)
    p->mem_size = p->pos + size;

  memcpy(&p->mem[p->pos], buffer, wbytes);
  p->pos         += wbytes;
  p->cached_size  = -1;

  return wbytes;
}

void
mm_mem_io_c::close() {
  close_mem_io();
}

void
mm_mem_io_c::close_mem_io() {
  auto p = p_func();

  if (p->free_mem)
    safefree(p->mem);
  p->mem       = nullptr;
  p->ro_mem    = nullptr;
  p->read_only = true;
  p->free_mem  = false;
  p->mem_size  = 0;
  p->increase  = 0;
  p->pos       = 0;
}

bool
mm_mem_io_c::eof() {
  auto p = p_func();

  return p->pos >= p->mem_size;
}

uint8_t *
mm_mem_io_c::get_buffer()
  const {
  auto p = p_func();

  if (p->read_only)
    throw mtx::invalid_parameter_x();
  return p->mem;
}

uint8_t const *
mm_mem_io_c::get_ro_buffer()
  const {
  auto p = p_func();

  if (!p->read_only)
    throw mtx::invalid_parameter_x();
  return p->ro_mem;
}

memory_cptr
mm_mem_io_c::get_and_lock_buffer() {
  auto p      = p_func();
  p->free_mem = false;

  return memory_c::take_ownership(p->mem, getFilePointer());
}

std::string
mm_mem_io_c::get_content()
  const {
  auto p      = p_func();
  auto source = p->read_only ? reinterpret_cast<char const *>(p->ro_mem) : reinterpret_cast<char const *>(p->mem);

  if (!source || !p->mem_size)
    return std::string{};

  return std::string(source, p->mem_size);
}

std::string
mm_mem_io_c::get_file_name()
  const {
  return p_func()->file_name;
}

void
mm_mem_io_c::set_file_name(const std::string &file_name) {
  p_func()->file_name = file_name;
}
