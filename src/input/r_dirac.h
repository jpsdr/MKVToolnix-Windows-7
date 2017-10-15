/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definitions for the Dirac demultiplexer module

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "merge/generic_reader.h"
#include "common/dirac.h"

class dirac_es_reader_c: public generic_reader_c {
private:
  dirac::sequence_header_t m_seqhdr;
  memory_cptr m_raw_seqhdr;

  memory_cptr m_buffer;

public:
  dirac_es_reader_c(const track_info_c &ti, const mm_io_cptr &in);

  virtual mtx::file_type_e get_format_type() const {
    return mtx::file_type_e::dirac;
  }

  virtual void read_headers();
  virtual file_status_e read(generic_packetizer_c *ptzr, bool force = false);
  virtual void identify();
  virtual void create_packetizer(int64_t id);
  virtual bool is_providing_timestamps() const {
    return false;
  }

  static int probe_file(mm_io_c *in, uint64_t size);
};
