/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   VobBtn packetizer

   Written by Steve Lhomme <steve.lhomme@free.fr>.
   Modified by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <matroska/KaxTracks.h>

#include "common/codec.h"
#include "common/compression.h"
#include "common/endian.h"
#include "merge/connection_checks.h"
#include "output/p_vobbtn.h"

vobbtn_packetizer_c::vobbtn_packetizer_c(generic_reader_c *p_reader,
                                         track_info_c &p_ti,
                                         int width,
                                         int height)
  : generic_packetizer_c{p_reader, p_ti, track_buttons}
  , m_previous_timestamp(0)
  , m_width(width)
  , m_height(height)
{
  set_default_compression_method(COMPRESSION_ZLIB);
}

vobbtn_packetizer_c::~vobbtn_packetizer_c() {
}

void
vobbtn_packetizer_c::set_headers() {
  set_codec_id(MKV_B_VOBBTN);

  set_video_pixel_dimensions(m_width, m_height);

  generic_packetizer_c::set_headers();

  m_track_entry->EnableLacing(false);
}

void
vobbtn_packetizer_c::process_impl(packet_cptr const &packet) {
  uint32_t vobu_start = get_uint32_be(packet->data->get_buffer() + 0x0d);
  uint32_t vobu_end   = get_uint32_be(packet->data->get_buffer() + 0x11);

  packet->duration = (int64_t)(100000.0 * (vobu_end - vobu_start) / 9);
  if (-1 == packet->timestamp) {
    packet->timestamp     = m_previous_timestamp;
    m_previous_timestamp += packet->duration;
  }

  packet->duration_mandatory = true;
  add_packet(packet);
}

connection_result_e
vobbtn_packetizer_c::can_connect_to(generic_packetizer_c *src,
                                    std::string &error_message) {
  vobbtn_packetizer_c *vsrc = dynamic_cast<vobbtn_packetizer_c *>(src);
  if (!vsrc)
    return CAN_CONNECT_NO_FORMAT;

  connect_check_v_width(m_width,   vsrc->m_width);
  connect_check_v_height(m_height, vsrc->m_height);

  return CAN_CONNECT_YES;
}
