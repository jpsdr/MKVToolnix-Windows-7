/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   PGS packetizer

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <matroska/KaxContentEncoding.h>
#include <matroska/KaxTracks.h>

#include "common/codec.h"
#include "common/compression.h"
#include "common/endian.h"
#include "common/hdmv_textst.h"
#include "merge/connection_checks.h"
#include "output/p_hdmv_textst.h"

using namespace libmatroska;

hdmv_textst_packetizer_c::hdmv_textst_packetizer_c(generic_reader_c *p_reader,
                                                   track_info_c &p_ti,
                                                   memory_cptr const &dialog_style_segment)
  : generic_packetizer_c{p_reader, p_ti}
{
  m_ti.m_private_data = dialog_style_segment->clone();
  set_track_type(track_subtitle);
  // set_default_compression_method(COMPRESSION_ZLIB);
}

hdmv_textst_packetizer_c::~hdmv_textst_packetizer_c() {
}

void
hdmv_textst_packetizer_c::set_headers() {
  set_codec_id(MKV_S_HDMV_TEXTST);
  set_codec_private(m_ti.m_private_data);

  generic_packetizer_c::set_headers();

  m_track_entry->EnableLacing(false);
}

int
hdmv_textst_packetizer_c::process(packet_cptr packet) {
  if ((packet->data->get_size() < 13) || (static_cast<mtx::hdmv_textst::segment_type_e>(packet->data->get_buffer()[0]) != mtx::hdmv_textst::dialog_presentation_segment))
    return FILE_STATUS_MOREDATA;

  auto buf       = packet->data->get_buffer();
  auto start_pts = timestamp_c::mpeg((static_cast<int64_t>(buf[3] & 1) << 32) | get_uint32_be(&buf[4]));
  auto end_pts   = timestamp_c::mpeg((static_cast<int64_t>(buf[8] & 1) << 32) | get_uint32_be(&buf[9]));

  if (!packet->has_timestamp())
    packet->timestamp = start_pts.to_ns();

  if (!packet->has_duration())
    packet->duration = (end_pts - start_pts).abs().to_ns();

  packet->duration_mandatory = true;

  add_packet(packet);

  return FILE_STATUS_MOREDATA;
}

connection_result_e
hdmv_textst_packetizer_c::can_connect_to(generic_packetizer_c *src,
                                         std::string &error_message) {
  if (!dynamic_cast<hdmv_textst_packetizer_c *>(src))
    return CAN_CONNECT_NO_FORMAT;

  connect_check_codec_private(src);

  return CAN_CONNECT_YES;
}
