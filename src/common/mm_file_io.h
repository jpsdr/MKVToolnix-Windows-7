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

class mm_file_io_c: public mm_io_c {
protected:
  std::string m_file_name;
  void *m_file;

#if defined(SYS_WINDOWS)
  bool m_eof;
#endif

public:
  mm_file_io_c(const std::string &path, const open_mode mode = MODE_READ);
  virtual ~mm_file_io_c();

  static void prepare_path(const std::string &path);
  static memory_cptr slurp(std::string const &file_name);

  virtual uint64 getFilePointer();
#if defined(SYS_WINDOWS)
  virtual uint64 get_real_file_pointer();
#endif
  virtual void setFilePointer(int64 offset, seek_mode mode = seek_beginning);
  virtual void close();
  virtual bool eof();
  virtual void clear_eof();

  virtual std::string get_file_name() const {
    return m_file_name;
  }

  virtual int truncate(int64_t pos);

  static void setup();
  static void cleanup();
  static mm_io_cptr open(const std::string &path, const open_mode mode = MODE_READ);

protected:
  virtual uint32 _read(void *buffer, size_t size);
  virtual size_t _write(const void *buffer, size_t size);
};
