/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definitions for the AVC/H.264 ES demultiplexer module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/debugging.h"
#include "merge/generic_reader.h"

class avc_es_reader_c: public generic_reader_c {
protected:
  static debugging_option_c ms_debug;

  int m_width{}, m_height{};
  int64_t m_default_duration{};

  memory_cptr m_buffer{memory_c::alloc(1024 * 1024)};

public:
  virtual mtx::file_type_e get_format_type() const {
    return mtx::file_type_e::avc_es;
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
