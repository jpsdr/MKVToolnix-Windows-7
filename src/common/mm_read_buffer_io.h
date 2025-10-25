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

class mm_read_buffer_io_private_c;
class mm_read_buffer_io_c: public mm_proxy_io_c {
protected:
  MTX_DECLARE_PRIVATE(mm_read_buffer_io_private_c)

  explicit mm_read_buffer_io_c(mm_read_buffer_io_private_c &p);

public:
  mm_read_buffer_io_c(mm_io_cptr const &in, std::size_t buffer_size = 1 << 17);
  virtual ~mm_read_buffer_io_c();

  virtual uint64_t getFilePointer() override;
  virtual void setFilePointer(int64_t offset, libebml::seek_mode mode = libebml::seek_beginning) override;
  virtual int64_t get_size() override;
  virtual bool eof() override;
  virtual void clear_eof() override;
  virtual void enable_buffering(bool enable);
  virtual void set_buffer_size(std::size_t new_buffer_size = 1 << 17);

protected:
  virtual uint32_t _read(void *buffer, size_t size) override;
  virtual size_t _write(const void *buffer, size_t size) override;
};
