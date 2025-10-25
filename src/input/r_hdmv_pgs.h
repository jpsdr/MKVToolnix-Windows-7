/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definitions for the PGS demultiplexer module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/debugging.h"
#include "common/error.h"
#include "merge/generic_reader.h"

class hdmv_pgs_reader_c: public generic_reader_c {
private:
  debugging_option_c m_debug{"hdmv_pgs_reader"};

public:
  virtual mtx::file_type_e get_format_type() const {
    return mtx::file_type_e::pgssup;
  }

  virtual void read_headers();
  virtual void identify();
  virtual void create_packetizer(int64_t id);
  virtual bool is_simple_subtitle_container() {
    return true;
  }

  virtual bool probe_file() override;

protected:
  virtual file_status_e read(generic_packetizer_c *packetizer, bool force = false) override;
};
