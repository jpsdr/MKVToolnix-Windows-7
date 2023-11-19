/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definitions for the HEVC/H.265 ES demultiplexer module

*/

#pragma once

#include "common/common_pch.h"

#include "common/hevc/util.h"
#include "merge/generic_reader.h"

class hevc_es_reader_c: public generic_reader_c {
private:
  int m_width{}, m_height{};
  int64_t m_default_duration{};

  memory_cptr m_buffer{memory_c::alloc(1024 * 1024)};

public:
  virtual mtx::file_type_e get_format_type() const {
    return mtx::file_type_e::hevc_es;
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
