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

#include "common/mm_proxy_io.h"
#include "common/mm_proxy_io_p.h"

/*
   Proxy class that does I/O on a mm_io_c handed over in the ctor.
   Useful for e.g. doing text I/O on other I/Os (file, mem).
*/

mm_proxy_io_c::mm_proxy_io_c(mm_io_cptr const &proxy_io)
  : mm_io_c{*new mm_proxy_io_private_c{proxy_io}}
{
}

mm_proxy_io_c::mm_proxy_io_c(mm_proxy_io_private_c &p)
  : mm_io_c{p}
{
}

mm_proxy_io_c::~mm_proxy_io_c() {
  close_proxy_io();
}

void
mm_proxy_io_c::setFilePointer(int64_t offset,
                              libebml::seek_mode mode) {
  return p_func()->proxy_io->setFilePointer(offset, mode);
}

uint64_t
mm_proxy_io_c::getFilePointer() {
  return p_func()->proxy_io->getFilePointer();
}

void
mm_proxy_io_c::clear_eof() {
  p_func()->proxy_io->clear_eof();
}

bool
mm_proxy_io_c::eof() {
  return p_func()->proxy_io->eof();
}

std::string
mm_proxy_io_c::get_file_name()
  const {
  return p_func()->proxy_io->get_file_name();
}

mm_io_c *
mm_proxy_io_c::get_proxied()
  const {
  return p_func()->proxy_io.get();
}

void
mm_proxy_io_c::close() {
  close_proxy_io();
}

void
mm_proxy_io_c::close_proxy_io() {
  p_func()->proxy_io.reset();
}

uint32_t
mm_proxy_io_c::_read(void *buffer,
                     size_t size) {
  return p_func()->proxy_io->read(buffer, size);
}

size_t
mm_proxy_io_c::_write(const void *buffer,
                      size_t size) {
  auto p = p_func();

  p->cached_size = -1;
  return p->proxy_io->write(buffer, size);
}
