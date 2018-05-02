/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   AV1 video output module

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/av1.h"
#include "common/codec.h"
#include "common/hacks.h"
#include "merge/connection_checks.h"
#include "output/p_av1.h"

using namespace libmatroska;

av1_video_packetizer_c::av1_video_packetizer_c(generic_reader_c *p_reader,
                                               track_info_c &p_ti)
  : generic_packetizer_c{p_reader, p_ti}
{
  if (!mtx::hacks::is_engaged(mtx::hacks::ENABLE_AV1))
    mxerror(Y("Support for AV1 is currently experimental and must be enabled with '--engage enable_av1' as the AV1 bitstream specification hasn't been finalized yet.\n"));

  m_timestamp_factory_application_mode = TFA_SHORT_QUEUEING;

  set_track_type(track_video);
  set_codec_id(MKV_V_AV1);
}

int
av1_video_packetizer_c::process(packet_cptr packet) {
  if (m_is_framed)
    process_framed(packet);
  else
    process_unframed(packet);

  return FILE_STATUS_MOREDATA;
}

void
av1_video_packetizer_c::process_framed(packet_cptr packet) {
  m_parser.debug_obu_types(*packet->data);

  packet->bref         = m_parser.is_keyframe(*packet->data) ? -1 : m_previous_timestamp;
  m_previous_timestamp = packet->timestamp;

  add_packet(packet);
}

void
av1_video_packetizer_c::process_unframed(packet_cptr packet) {
  m_parser.debug_obu_types(*packet->data);

  m_parser.parse(*packet->data);
  flush_frames();
}

void
av1_video_packetizer_c::flush_impl() {
  if (m_is_framed)
    return;

  m_parser.flush();
  flush_frames();
}

void
av1_video_packetizer_c::flush_frames() {
  while (m_parser.frame_available()) {
    auto frame           = m_parser.get_next_frame();
    auto bref            = frame.is_keyframe ? -1 : m_previous_timestamp;
    m_previous_timestamp = frame.timestamp;

    add_packet(std::make_shared<packet_t>(frame.mem, frame.timestamp, -1, bref));
  }
}

void
av1_video_packetizer_c::set_is_unframed() {
  m_is_framed = false;
}

connection_result_e
av1_video_packetizer_c::can_connect_to(generic_packetizer_c *src,
                                       std::string &error_message) {
  auto psrc = dynamic_cast<av1_video_packetizer_c *>(src);
  if (!psrc)
    return CAN_CONNECT_NO_FORMAT;

  connect_check_v_width( m_hvideo_pixel_width,  psrc->m_hvideo_pixel_width);
  connect_check_v_height(m_hvideo_pixel_height, psrc->m_hvideo_pixel_height);

  return CAN_CONNECT_YES;
}

bool
av1_video_packetizer_c::is_compatible_with(output_compatibility_e compatibility) {
  return (OC_MATROSKA == compatibility) || (OC_WEBM == compatibility);
}
