/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definitions for the IVF demultiplexer module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/codec.h"
#include "common/ivf.h"
#include "common/mm_io.h"
#include "merge/generic_reader.h"
#include "merge/block_addition_mapping.h"

class ivf_reader_c: public generic_reader_c {
private:
  ivf::file_header_t m_header;
  uint16_t m_width{}, m_height{};
  uint64_t m_frame_rate_num{}, m_frame_rate_den{};
  codec_c m_codec;
  bool m_ok{};
  debugging_option_c m_debug{"ivf_reader"};

  std::vector<block_addition_mapping_t> m_block_addition_mappings;

public:
  virtual mtx::file_type_e get_format_type() const {
    return mtx::file_type_e::ivf;
  }

  virtual void add_available_track_ids() override;
  virtual void read_headers() override;
  virtual void identify() override;
  virtual void create_packetizer(int64_t id) override;

  virtual bool probe_file() override;

protected:
  virtual file_status_e read(generic_packetizer_c *packetizer, bool force = false) override;

  void create_av1_packetizer();
  void create_vpx_packetizer();

  void parse_first_av1_frame();
};
