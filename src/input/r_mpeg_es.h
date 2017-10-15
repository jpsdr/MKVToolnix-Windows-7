/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definitions for the MPEG ES/PS demultiplexer module

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/dts.h"
#include "common/mpeg1_2.h"
#include "merge/generic_reader.h"
#include "mpegparser/M2VParser.h"

class mpeg_es_reader_c: public generic_reader_c {
private:
  int version, interlaced, width, height, dwidth, dheight;
  double frame_rate, aspect_ratio;

public:
  mpeg_es_reader_c(const track_info_c &ti, const mm_io_cptr &in);
  virtual ~mpeg_es_reader_c();

  virtual mtx::file_type_e get_format_type() const {
    return mtx::file_type_e::mpeg_es;
  }

  virtual void read_headers();
  virtual file_status_e read(generic_packetizer_c *ptzr, bool force = false);
  virtual void identify();
  virtual void create_packetizer(int64_t id);
  virtual bool is_providing_timestamps() const {
    return false;
  }

  static bool read_frame(M2VParser &parser, mm_io_c &in, int64_t max_size = -1);

  static int probe_file(mm_io_c *in, uint64_t size);
};
