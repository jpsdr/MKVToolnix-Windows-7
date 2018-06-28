/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definitions for the AV1 Open Bistream Unit (OBU) demultiplexer module

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "merge/generic_reader.h"

class obu_reader_c: public generic_reader_c {
protected:
  unsigned int m_width{}, m_height{};
  memory_cptr m_buffer;

public:
  obu_reader_c(const track_info_c &ti, const mm_io_cptr &in);

  virtual mtx::file_type_e get_format_type() const override {
    return mtx::file_type_e::obu;
  }

  virtual void read_headers() override;
  virtual void identify() override;
  virtual void create_packetizer(int64_t id) override;
  virtual bool is_providing_timestamps() const override {
    return false;
  }

  static bool probe_file(mm_io_c &in, uint64_t size);

protected:
  virtual file_status_e read(generic_packetizer_c *ptzr, bool force = false) override;
};
