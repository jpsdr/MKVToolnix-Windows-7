/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definitions for the MPEG ES/PS demultiplexer module

   Written by Moritz Bunkus <mo@bunkus.online>.
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
  mtx_mp_rational_t field_duration, aspect_ratio;

public:
  virtual mtx::file_type_e get_format_type() const {
    return mtx::file_type_e::mpeg_es;
  }

  virtual void read_headers();
  virtual void identify();
  virtual void create_packetizer(int64_t id);
  virtual bool is_providing_timestamps() const {
    return false;
  }

  virtual bool probe_file() override;

protected:
  virtual file_status_e read(generic_packetizer_c *packetizer, bool force = false) override;

  static bool read_frame(M2VParser &parser, mm_io_c &in, int64_t max_size = -1);
};
