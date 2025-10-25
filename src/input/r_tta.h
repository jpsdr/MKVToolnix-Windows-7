/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definitions for the TTA demultiplexer module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/error.h"
#include "merge/generic_reader.h"
#include "common/tta.h"

class tta_reader_c: public generic_reader_c {
private:
  memory_cptr m_chunk;
  std::vector<uint32_t> seek_points;
  unsigned int pos;
  mtx::tta::file_header_t header;

public:
  virtual mtx::file_type_e get_format_type() const {
    return mtx::file_type_e::tta;
  }

  virtual void read_headers();
  virtual void identify();
  virtual void create_packetizer(int64_t id);

  virtual bool probe_file() override;

protected:
  virtual file_status_e read(generic_packetizer_c *packetizer, bool force = false) override;
};
