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

class mm_null_io_private_c;
class mm_null_io_c: public mm_io_c {
protected:
  MTX_DECLARE_PRIVATE(mm_null_io_private_c);

  explicit mm_null_io_c(mm_null_io_private_c &p);

public:
  mm_null_io_c(std::string const &file_name);

  virtual uint64 getFilePointer();
  virtual void setFilePointer(int64 offset, libebml::seek_mode mode = libebml::seek_beginning);
  virtual void close();
  virtual bool eof();
  virtual std::string get_file_name() const;

protected:
  virtual uint32 _read(void *buffer, size_t size);
  virtual size_t _write(const void *buffer, size_t size);
};
