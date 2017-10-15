/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definitions for the IVF demultiplexer module

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/codec.h"
#include "common/ivf.h"
#include "common/mm_io.h"
#include "merge/generic_reader.h"

class ivf_reader_c: public generic_reader_c {
private:
  uint16_t m_width, m_height;
  uint64_t m_frame_rate_num, m_frame_rate_den;
  codec_c m_codec;

public:
  ivf_reader_c(const track_info_c &ti, const mm_io_cptr &in);
  virtual ~ivf_reader_c();

  virtual mtx::file_type_e get_format_type() const {
    return mtx::file_type_e::ivf;
  }

  virtual void read_headers();
  virtual file_status_e read(generic_packetizer_c *ptzr, bool force = false);
  virtual void identify();
  virtual void create_packetizer(int64_t id);

  static int probe_file(mm_io_c *in, uint64_t size);
};
