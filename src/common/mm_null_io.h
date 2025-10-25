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

class mm_null_io_private_c;
class mm_null_io_c: public mm_io_c {
protected:
  MTX_DECLARE_PRIVATE(mm_null_io_private_c)

  explicit mm_null_io_c(mm_null_io_private_c &p);

public:
  mm_null_io_c(std::string const &file_name);

  virtual uint64_t getFilePointer() override;
  virtual void setFilePointer(int64_t offset, libebml::seek_mode mode = libebml::seek_beginning) override;
  virtual void close() override;
  virtual bool eof() override;
  virtual std::string get_file_name() const override;

protected:
  virtual uint32_t _read(void *buffer, size_t size) override;
  virtual size_t _write(const void *buffer, size_t size) override;
};
