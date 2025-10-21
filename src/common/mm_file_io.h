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

#include <ebml/StdIOCallback.h>

class mm_file_io_private_c;
class mm_file_io_c: public mm_io_c {
protected:
  MTX_DECLARE_PRIVATE(mm_file_io_private_c)

  explicit mm_file_io_c(mm_file_io_private_c &p);

public:
  mm_file_io_c(const std::string &path, const libebml::open_mode mode = libebml::MODE_READ);
  virtual ~mm_file_io_c();

  virtual uint64_t getFilePointer() override;
#if defined(SYS_WINDOWS)
  virtual uint64_t get_real_file_pointer();
#endif
  virtual void setFilePointer(int64_t offset, libebml::seek_mode mode = libebml::seek_beginning) override;
  virtual void close() override;
  virtual bool eof() override;
  virtual void clear_eof() override;
  virtual int truncate(int64_t pos) override;

  virtual std::string get_file_name() const override;

public:
  static void prepare_path(const std::string &path);
  static memory_cptr slurp(std::string const &file_name);

  static mm_io_cptr open(const std::string &path, const libebml::open_mode mode = libebml::MODE_READ);

  static void enable_flushing_on_close(bool enable);

protected:
  virtual uint32_t _read(void *buffer, size_t size) override;
  virtual size_t _write(const void *buffer, size_t size) override;
};
