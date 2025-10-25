/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   PGS demultiplexer module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/codec.h"
#include "common/endian.h"
#include "common/mm_io_x.h"
#include "common/hdmv_pgs.h"
#include "common/strings/formatting.h"
#include "input/r_hdmv_pgs.h"
#include "output/p_hdmv_pgs.h"
#include "merge/file_status.h"
#include "merge/input_x.h"

bool
hdmv_pgs_reader_c::probe_file() {
  if (mtx::hdmv_pgs::FILE_MAGIC != m_in->read_uint16_be())
    return false;

  m_in->skip(4 + 4 + 1);
  auto segment_size = m_in->read_uint16_be();

  m_in->setFilePointer(segment_size, libebml::seek_current);

  return mtx::hdmv_pgs::FILE_MAGIC == m_in->read_uint16_be();
}

void
hdmv_pgs_reader_c::read_headers() {
  show_demuxer_info();
}

void
hdmv_pgs_reader_c::create_packetizer(int64_t) {
  if (!demuxing_requested('s', 0) || !m_reader_packetizers.empty())
    return;

  auto packetizer = new hdmv_pgs_packetizer_c(this, m_ti);
  packetizer->set_aggregate_packets(true);
  add_packetizer(packetizer);

  show_packetizer_info(0, *packetizer);
}

file_status_e
hdmv_pgs_reader_c::read(generic_packetizer_c *,
                        bool) {
  try {
    auto file_position = m_in->getFilePointer();

    if (mtx::hdmv_pgs::FILE_MAGIC != m_in->read_uint16_be())
      return flush_packetizers();

    auto timestamp_raw = static_cast<uint64_t>(m_in->read_uint32_be());
    auto timestamp     = timestamp_raw * 100000Lu / 9;

    mxdebug_if(m_debug, fmt::format("hdmv_pgs_reader_c::read(): ---------- start read at {0} timestamp_raw {1} (0x{1:08x}) = {2}\n", file_position, timestamp_raw, mtx::string::format_timestamp(timestamp)));

    m_in->skip(4);

    memory_cptr frame = memory_c::alloc(3);
    if (3 != m_in->read(frame->get_buffer(), 3))
      return flush_packetizers();

    unsigned int segment_size = get_uint16_be(frame->get_buffer() + 1);
    frame->resize(3 + segment_size);

    if (segment_size != m_in->read(frame->get_buffer() + 3, segment_size))
      return flush_packetizers();

    mxdebug_if(m_debug, fmt::format("hdmv_pgs_reader_c::read(): type {0:02x} size {1} at {2}\n", static_cast<unsigned int>(frame->get_buffer()[0]), segment_size, m_in->getFilePointer() - 10 - 3));

    ptzr(0).process(std::make_shared<packet_t>(frame, timestamp));

  } catch (...) {
    mxdebug_if(m_debug, "hdmv_pgs_reader_c::read(): exception\n");

    return flush_packetizers();
  }

  return FILE_STATUS_MOREDATA;
}

void
hdmv_pgs_reader_c::identify() {
  id_result_container();
  id_result_track(0, ID_RESULT_TRACK_SUBTITLES, codec_c::get_name(codec_c::type_e::S_HDMV_PGS, "HDMV PGS"));
}
