/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definitions for the HDMV TextST demultiplexer module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "merge/generic_reader.h"

class hdmv_textst_reader_c: public generic_reader_c {
private:
  memory_cptr m_dialog_style_segment;

public:
  virtual mtx::file_type_e get_format_type() const {
    return mtx::file_type_e::hdmv_textst;
  }

  virtual void read_headers();
  virtual void identify();
  virtual void create_packetizer(int64_t id);
  virtual bool is_simple_subtitle_container() {
    return true;
  }

  virtual bool probe_file() override;

protected:
  virtual file_status_e read(generic_packetizer_c *packetizer, bool force = false) override;

  static memory_cptr read_segment(mm_io_c &in);
};
