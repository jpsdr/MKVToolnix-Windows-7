/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definitions for the DTS demultiplexer module

   Written by Peter Niemayer <niemayer@isg.de>.
   Modified by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/dts.h"
#include "common/error.h"
#include "common/mm_io.h"
#include "merge/generic_reader.h"

class dts_reader_c: public generic_reader_c {
private:
  enum class chunk_type_e: uint64_t {
      dtshdhdr = 0x4454534844484452ull
    , strmdata = 0x5354524d44415441ull
  };

  struct chunk_t {
    chunk_type_e type;
    uint64_t data_start, data_size, data_end;

    chunk_t(chunk_type_e p_type,
            uint64_t p_data_start,
            uint64_t p_data_size)
      : type{p_type}
      , data_start{p_data_start}
      , data_size{p_data_size}
      , data_end{data_start + data_size}
    {
    }
  };

  using chunks_t = std::vector<chunk_t>;

  memory_cptr m_af_buf;
  unsigned short *m_buf[2];
  unsigned int m_cur_buf;
  mtx::dts::header_t m_dtsheader;
  bool m_dts14_to_16, m_swap_bytes;
  debugging_option_c m_debug;
  codec_c m_codec;
  chunks_t m_chunks;
  chunks_t::const_iterator m_current_chunk;

public:
  dts_reader_c(const track_info_c &ti, const mm_io_cptr &in);
  virtual ~dts_reader_c();

  virtual mtx::file_type_e get_format_type() const {
    return mtx::file_type_e::dts;
  }

  virtual void read_headers();
  virtual file_status_e read(generic_packetizer_c *ptzr, bool force = false);
  virtual void identify();
  virtual void create_packetizer(int64_t id);
  virtual bool is_providing_timestamps() const {
    return false;
  }

  static int probe_file(mm_io_c *in, uint64_t size, bool strict_mode = false);
  static chunks_t scan_chunks(mm_io_c &in);

protected:
  virtual int decode_buffer(size_t length);
};
