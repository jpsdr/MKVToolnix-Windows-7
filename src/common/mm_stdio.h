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

class mm_stdio_c: public mm_io_c {
public:
  mm_stdio_c() = default;

  virtual uint64_t getFilePointer();
  virtual void setFilePointer(int64_t offset, libebml::seek_mode mode=libebml::seek_beginning);
  virtual void close();
  virtual bool eof() {
    return false;
  }
  virtual std::string get_file_name() const {
    return "";
  }
  virtual void flush();

#if defined(SYS_WINDOWS)
  virtual void set_string_output_converter(charset_converter_cptr const &) {
  }
#endif

protected:
  virtual uint32_t _read(void *buffer, size_t size);
  virtual size_t _write(const void *buffer, size_t size);
};
