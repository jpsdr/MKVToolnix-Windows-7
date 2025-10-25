/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   PGS packetizer

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <matroska/KaxTracks.h>

#include "common/codec.h"
#include "common/compression.h"
#include "common/endian.h"
#include "common/hdmv_pgs.h"
#include "common/strings/formatting.h"
#include "output/p_hdmv_pgs.h"

hdmv_pgs_packetizer_c::hdmv_pgs_packetizer_c(generic_reader_c *p_reader,
                                   track_info_c &p_ti)
  : generic_packetizer_c{p_reader, p_ti, track_subtitle}
  , m_aggregate_packets(false)
{
  set_default_compression_method(COMPRESSION_ZLIB);
}

hdmv_pgs_packetizer_c::~hdmv_pgs_packetizer_c() {
}

void
hdmv_pgs_packetizer_c::set_headers() {
  set_codec_id(MKV_S_HDMV_PGS);

  generic_packetizer_c::set_headers();

  m_track_entry->EnableLacing(false);
}

void
hdmv_pgs_packetizer_c::process_impl(packet_cptr const &packet) {
  packet->force_key_frame();

  if (!m_aggregate_packets) {
    dump_and_add_packet(packet);
    return;
  }

  if (!m_aggregated) {
    m_aggregated = packet;
    m_aggregated->data->take_ownership();

  } else
    m_aggregated->data->add(packet->data);

  if (   (0                                     != packet->data->get_size())
      && (mtx::hdmv_pgs::END_OF_DISPLAY_SEGMENT == packet->data->get_buffer()[0])) {
    dump_and_add_packet(m_aggregated);
    m_aggregated.reset();
  }
}

void
hdmv_pgs_packetizer_c::dump_and_add_packet(packet_cptr const &packet) {
  if (m_debug)
    dump_packet(*packet);

  add_packet(packet);
}

void
hdmv_pgs_packetizer_c::dump_packet(packet_t const &packet) {
  auto ptr        = packet.data->get_buffer();
  auto frame_size = packet.data->get_size();
  auto offset     = 0u;

  mxdebug(fmt::format("HDMV PGS frame {0} size {1} timestamp {2} duration {3}\n", m_num_packets, frame_size, mtx::string::format_timestamp(packet.timestamp), mtx::string::format_timestamp(packet.duration)));

  while ((offset + 3) <= frame_size) {
    auto segment_size  = std::min<unsigned int>(get_uint16_be(ptr + offset + 1) + 3, frame_size - offset);
    auto type          = ptr[offset];
    offset            += segment_size;

    mxdebug(fmt::format("  segment size {0} at {1} type 0x{2:02x} ({3}){4}\n",
                        segment_size, offset - segment_size, type, mtx::hdmv_pgs::name_for_type(type),
                        offset > frame_size ? " INVALID SEGMENT SIZE" : ""));
  }
}

connection_result_e
hdmv_pgs_packetizer_c::can_connect_to(generic_packetizer_c *src,
                                      std::string &) {
  return !dynamic_cast<hdmv_pgs_packetizer_c *>(src) ? CAN_CONNECT_NO_FORMAT : CAN_CONNECT_YES;
}
