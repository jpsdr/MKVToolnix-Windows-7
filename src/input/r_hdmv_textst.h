/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definitions for the HDMV TextST demultiplexer module

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "merge/generic_reader.h"

class hdmv_textst_reader_c: public generic_reader_c {
private:
  memory_cptr m_dialog_style_segment;

public:
  hdmv_textst_reader_c(const track_info_c &ti, const mm_io_cptr &in);
  virtual ~hdmv_textst_reader_c();

  virtual mtx::file_type_e get_format_type() const {
    return mtx::file_type_e::hdmv_textst;
  }

  virtual void read_headers();
  virtual file_status_e read(generic_packetizer_c *ptzr, bool force = false);
  virtual void identify();
  virtual void create_packetizer(int64_t id);
  virtual bool is_simple_subtitle_container() {
    return true;
  }

  static int probe_file(mm_io_c *in, uint64_t size);

protected:
  static memory_cptr read_segment(mm_io_c &in);
};
