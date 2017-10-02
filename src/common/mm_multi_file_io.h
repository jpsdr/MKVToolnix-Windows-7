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

class mm_multi_file_io_c;
using mm_multi_file_io_cptr = std::shared_ptr<mm_multi_file_io_c>;

class mm_multi_file_io_c: public mm_io_c {
protected:
  struct file_t {
    bfs::path m_file_name;
    uint64_t m_size, m_global_start;
    mm_file_io_cptr m_file;

    file_t(const bfs::path &file_name, uint64_t global_start, mm_file_io_cptr file);
  };

protected:
  std::string m_display_file_name;
  uint64_t m_total_size, m_current_pos, m_current_local_pos;
  unsigned int m_current_file;
  std::vector<mm_multi_file_io_c::file_t> m_files;

public:
  mm_multi_file_io_c(const std::vector<bfs::path> &file_names, const std::string &display_file_name);
  virtual ~mm_multi_file_io_c();

  virtual uint64 getFilePointer();
  virtual void setFilePointer(int64 offset, seek_mode mode = seek_beginning);
  virtual void close();
  virtual bool eof();

  virtual std::string get_file_name() const {
    return m_display_file_name;
  }
  virtual std::vector<bfs::path> get_file_names();
  virtual void create_verbose_identification_info(mtx::id::info_c &info);
  virtual void display_other_file_info();
  virtual void enable_buffering(bool enable);

  static mm_io_cptr open_multi(const std::string &display_file_name, bool single_only = false);

protected:
  virtual uint32 _read(void *buffer, size_t size);
  virtual size_t _write(const void *buffer, size_t size);
};
