/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   IO callback class definitions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

class mm_mem_io_private_c;
class mm_mem_io_c: public mm_io_c {
protected:
  MTX_DECLARE_PRIVATE(mm_mem_io_private_c)

  explicit mm_mem_io_c(mm_mem_io_private_c &p);

public:
  mm_mem_io_c(uint8_t *mem, uint64_t mem_size, std::size_t increase);
  mm_mem_io_c(const uint8_t *mem, uint64_t mem_size);
  mm_mem_io_c(memory_c const &mem);
  virtual ~mm_mem_io_c();

  virtual uint64_t getFilePointer() override;
  virtual void setFilePointer(int64_t offset, libebml::seek_mode mode = libebml::seek_beginning) override;
  virtual void close() override;
  virtual bool eof() override;
  virtual std::string get_file_name() const override;
  virtual void set_file_name(const std::string &file_name);

  virtual uint8_t *get_buffer() const;
  virtual uint8_t const *get_ro_buffer() const;
  virtual memory_cptr get_and_lock_buffer();
  virtual std::string get_content() const;

protected:
  virtual uint32_t _read(void *buffer, size_t size) override;
  virtual size_t _write(const void *buffer, size_t size) override;
  void close_mem_io();
};
