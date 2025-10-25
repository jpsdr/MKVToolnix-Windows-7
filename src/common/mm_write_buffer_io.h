/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   IO callback class definitions

   Written by Moritz Bunkus <mo@bunkus.online>.
   Modifications by Nils Maier <maierman@web.de>
*/

#pragma once

#include "common/common_pch.h"

#include "common/mm_io.h"

class mm_write_buffer_io_private_c;
class mm_write_buffer_io_c: public mm_proxy_io_c {
protected:
  MTX_DECLARE_PRIVATE(mm_write_buffer_io_private_c)

  explicit mm_write_buffer_io_c(mm_write_buffer_io_private_c &p);

public:
  mm_write_buffer_io_c(mm_io_cptr const &out, std::size_t buffer_size);
  virtual ~mm_write_buffer_io_c();

  virtual uint64_t getFilePointer() override;
  virtual void setFilePointer(int64_t offset, libebml::seek_mode mode = libebml::seek_beginning) override;
  virtual void flush() override;
  virtual void close() override;
  virtual void discard_buffer();

  static mm_io_cptr open(const std::string &file_name, size_t buffer_size);

protected:
  virtual uint32_t _read(void *buffer, size_t size) override;
  virtual size_t _write(const void *buffer, size_t size) override;
  void flush_buffer();
  void close_write_buffer_io();
};
