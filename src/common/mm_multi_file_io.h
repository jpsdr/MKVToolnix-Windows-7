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

#include "common/mm_io.h"

namespace mtx { namespace id {
class info_c;
}}

class mm_multi_file_io_private_c;
class mm_multi_file_io_c: public mm_io_c {
protected:
  MTX_DECLARE_PRIVATE(mm_multi_file_io_private_c)

  explicit mm_multi_file_io_c(mm_multi_file_io_private_c &p);

public:
  mm_multi_file_io_c(std::vector<bfs::path> const &file_names, std::string const &display_file_name);
  virtual ~mm_multi_file_io_c();

  virtual uint64 getFilePointer();
  virtual void setFilePointer(int64 offset, libebml::seek_mode mode = libebml::seek_beginning);
  virtual void close();
  virtual bool eof();

  virtual std::string get_file_name() const;
  virtual std::vector<bfs::path> get_file_names();
  virtual void create_verbose_identification_info(mtx::id::info_c &info);
  virtual void display_other_file_info();
  virtual void enable_buffering(bool enable);

  static mm_io_cptr open_multi(const std::string &display_file_name, bool single_only = false);

protected:
  virtual uint32 _read(void *buffer, size_t size);
  virtual size_t _write(const void *buffer, size_t size);

  void close_multi_file_io();
};
