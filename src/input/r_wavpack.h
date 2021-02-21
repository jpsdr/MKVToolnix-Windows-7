/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definitions for the TTA demultiplexer module

   Written by Steve Lhomme <steve.lhomme@free.fr>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/error.h"
#include "merge/generic_reader.h"
#include "common/wavpack.h"

class wavpack_reader_c: public generic_reader_c {
private:
  mm_io_cptr m_in_correc;
  mtx::wavpack::header_t header, header_correc;
  mtx::wavpack::meta_t meta, meta_correc;

public:
  virtual mtx::file_type_e get_format_type() const {
    return mtx::file_type_e::wavpack4;
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
};
