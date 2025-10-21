/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   AV1 video output module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/av1.h"
#include "common/codec.h"
#include "merge/output_control.h"
#include "output/p_av1.h"

av1_video_packetizer_c::av1_video_packetizer_c(generic_reader_c *p_reader,
                                               track_info_c &p_ti,
                                               unsigned int width,
                                               unsigned int height)
  : generic_packetizer_c{p_reader, p_ti, track_video}
{
  m_timestamp_factory_application_mode = TFA_SHORT_QUEUEING;

  set_codec_id(MKV_V_AV1);
  set_codec_private(m_ti.m_private_data);
  set_video_pixel_dimensions(width, height);
}

void
av1_video_packetizer_c::set_header_parameters() {
  if (m_header_parameters_set)
    return;

  if (!m_parser.headers_parsed())
    return;

  auto need_to_rerender = false;

  if (!m_hcodec_private) {
    set_codec_private(m_parser.get_av1c());
    need_to_rerender = true;
  }

  auto dimensions = m_parser.get_pixel_dimensions();

  if (!m_hvideo_pixel_width || !m_hvideo_pixel_height) {
    set_video_pixel_dimensions(dimensions.first, dimensions.second);
    need_to_rerender = true;
  }

  if (!m_hvideo_display_width || !m_hvideo_display_height) {
    set_video_display_dimensions(dimensions.first, dimensions.second, generic_packetizer_c::ddu_pixels, option_source_e::bitstream);
    need_to_rerender = true;
  }

  auto frame_duration = m_parser.get_frame_duration();
  if ((m_htrack_default_duration <= 0) && frame_duration) {
    set_track_default_duration(mtx::to_int(frame_duration));
    need_to_rerender = true;

  } else if ((m_htrack_default_duration > 0) && !frame_duration)
    m_parser.set_default_duration(m_htrack_default_duration);

  if (need_to_rerender)
    rerender_track_headers();

  m_header_parameters_set = true;

  if (!m_is_framed)
    return;

  m_parser.set_parse_sequence_header_obus_only(true);

  while (m_parser.frame_available())
    m_parser.get_next_frame();
}

void
av1_video_packetizer_c::process_impl(packet_cptr const &packet) {
  m_parser.debug_obu_types(*packet->data);

  m_parser.parse(*packet->data);

  set_header_parameters();

  if (m_is_framed)
    process_framed(packet);
  else
    process_unframed();
}

void
av1_video_packetizer_c::process_framed(packet_cptr const &packet) {
  packet->bref         = m_parser.is_keyframe(*packet->data) ? -1 : m_previous_timestamp;
  m_previous_timestamp = packet->timestamp;

  add_packet(packet);
}

void
av1_video_packetizer_c::process_unframed() {
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
    auto duration        = m_htrack_default_duration > 0 ? m_htrack_default_duration : -1;
    m_previous_timestamp = frame.timestamp;

    add_packet(std::make_shared<packet_t>(frame.mem, frame.timestamp, duration, bref));
  }
}

void
av1_video_packetizer_c::set_is_unframed() {
  m_is_framed = false;
}

connection_result_e
av1_video_packetizer_c::can_connect_to(generic_packetizer_c *src,
                                       std::string &) {
  auto psrc = dynamic_cast<av1_video_packetizer_c *>(src);
  if (!psrc)
    return CAN_CONNECT_NO_FORMAT;

  return CAN_CONNECT_YES;
}

bool
av1_video_packetizer_c::is_compatible_with(output_compatibility_e compatibility) {
  return (OC_MATROSKA == compatibility) || (OC_WEBM == compatibility);
}
