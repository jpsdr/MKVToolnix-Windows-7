/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   IO callback class definitions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

class mm_proxy_io_c: public mm_io_c {
protected:
  mm_io_cptr m_proxy_io;

public:
  mm_proxy_io_c(mm_io_cptr const &proxy_io)
    : m_proxy_io{proxy_io}
  {
  }
  virtual ~mm_proxy_io_c() {
    close();
  }

  virtual void setFilePointer(int64 offset, seek_mode mode=seek_beginning) {
    return m_proxy_io->setFilePointer(offset, mode);
  }
  virtual uint64 getFilePointer() {
    return m_proxy_io->getFilePointer();
  }
  virtual void clear_eof() {
    m_proxy_io->clear_eof();
  }
  virtual bool eof() {
    return m_proxy_io->eof();
  }
  virtual void close();
  virtual std::string get_file_name() const {
    return m_proxy_io->get_file_name();
  }
  virtual mm_io_c *get_proxied() const {
    return m_proxy_io.get();
  }

protected:
  virtual uint32 _read(void *buffer, size_t size);
  virtual size_t _write(const void *buffer, size_t size);
};
