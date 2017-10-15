/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definitions for the TrueHD demultiplexer module

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/error.h"
#include "common/mm_io.h"
#include "input/truehd_ac3_splitting_packet_converter.h"
#include "merge/generic_reader.h"
#include "common/ac3.h"
#include "common/truehd.h"

class truehd_reader_c: public generic_reader_c {
private:
  memory_cptr m_chunk;
  truehd_frame_cptr m_header;
  int m_truehd_ptzr, m_ac3_ptzr;
  mtx::ac3::frame_c m_ac3_header;
  truehd_ac3_splitting_packet_converter_c m_converter;

public:
  truehd_reader_c(const track_info_c &ti, const mm_io_cptr &in);
  virtual ~truehd_reader_c();

  virtual mtx::file_type_e get_format_type() const {
    return mtx::file_type_e::truehd;
  }

  virtual void read_headers();
  virtual file_status_e read(generic_packetizer_c *ptzr, bool force = false);
  virtual void identify();
  virtual void create_packetizers();
  virtual void create_packetizer(int64_t tid);
  virtual void add_available_track_ids();
  virtual bool is_providing_timestamps() const {
    return false;
  }

  static int probe_file(mm_io_c *in, uint64_t size);

protected:
  static bool find_valid_headers(mm_io_c &in, int64_t probe_range, int num_headers);
};
