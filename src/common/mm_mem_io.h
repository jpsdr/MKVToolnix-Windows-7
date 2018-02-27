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

class mm_mem_io_c: public mm_io_c {
protected:
  size_t m_pos, m_mem_size, m_allocated, m_increase;
  unsigned char *m_mem;
  const unsigned char *m_ro_mem;
  bool m_free_mem, m_read_only;
  std::string m_file_name;

public:
  mm_mem_io_c(unsigned char *mem, uint64_t mem_size, int increase);
  mm_mem_io_c(const unsigned char *mem, uint64_t mem_size);
  mm_mem_io_c(memory_c const &mem);
  ~mm_mem_io_c();

  virtual uint64 getFilePointer();
  virtual void setFilePointer(int64 offset, seek_mode mode = seek_beginning);
  virtual void close();
  virtual bool eof();
  virtual std::string get_file_name() const {
    return m_file_name;
  }
  virtual void set_file_name(const std::string &file_name) {
    m_file_name = file_name;
  }

  virtual unsigned char *get_buffer() const;
  virtual unsigned char const *get_ro_buffer() const;
  virtual unsigned char *get_and_lock_buffer();
  virtual std::string get_content() const;

protected:
  virtual uint32 _read(void *buffer, size_t size);
  virtual size_t _write(const void *buffer, size_t size);
};
