/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definitions for the AC-3 demultiplexer module

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/ac3.h"
#include "common/error.h"
#include "common/mm_io.h"
#include "merge/generic_reader.h"

class ac3_reader_c: public generic_reader_c {
private:
  mtx::ac3::frame_c m_ac3header;
  memory_cptr m_chunk;

public:
  ac3_reader_c(const track_info_c &ti, const mm_io_cptr &in);
  virtual ~ac3_reader_c();

  virtual mtx::file_type_e get_format_type() const {
    return mtx::file_type_e::ac3;
  }

  virtual void read_headers();
  virtual file_status_e read(generic_packetizer_c *ptzr, bool force = false);
  virtual void identify();
  virtual void create_packetizer(int64_t id);
  virtual bool is_providing_timestamps() const {
    return false;
  }

  static int probe_file(mm_io_c *in, uint64_t size, int64_t probe_size, int num_headers, bool require_zero_offset = false);

protected:
  static int find_valid_headers(mm_io_c &in, int64_t probe_range, int num_headers);
};
