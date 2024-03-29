/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   VC1 ES video output module

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <matroska/KaxTracks.h>

#include "avilib.h"
#include "common/codec.h"
#include "common/endian.h"
#include "common/hacks.h"
#include "merge/output_control.h"
#include "merge/generic_reader.h"
#include "merge/packet_extensions.h"
#include "output/p_vc1.h"

vc1_video_packetizer_c::vc1_video_packetizer_c(generic_reader_c *n_reader, track_info_c &n_ti)
  : generic_packetizer_c{n_reader, n_ti, track_video}
  , m_previous_timestamp(-1)
{
  m_relaxed_timestamp_checking = true;

  set_codec_id(MKV_V_MSCOMP);

  // Dummy values
  m_seqhdr.pixel_width  = 123;
  m_seqhdr.pixel_height = 123;
}

void
vc1_video_packetizer_c::set_headers() {
  int priv_size = sizeof(alBITMAPINFOHEADER) + (m_raw_headers ? m_raw_headers->get_size() + 1 : 0);
  auto buf      = memory_c::alloc(priv_size);
  auto bih      = reinterpret_cast<alBITMAPINFOHEADER *>(buf->get_buffer());

  memset(bih, 0, priv_size);
  memcpy(&bih->bi_compression, "WVC1", 4);

  put_uint32_le(&bih->bi_size,             priv_size);
  put_uint16_le(&bih->bi_planes,           1);
  put_uint16_le(&bih->bi_bit_count,        24);
  put_uint32_le(&bih->bi_width,            m_seqhdr.pixel_width);
  put_uint32_le(&bih->bi_height,           m_seqhdr.pixel_height);
  put_uint32_le(&bih->bi_size_image,       get_uint32_le(&bih->bi_width) * get_uint32_le(&bih->bi_height) * 3);
  put_uint32_le(&bih->bi_x_pels_per_meter, 1);
  put_uint32_le(&bih->bi_y_pels_per_meter, 1);

  set_video_pixel_dimensions(m_seqhdr.pixel_width, m_seqhdr.pixel_height);

  if (m_raw_headers) {
    if (m_seqhdr.display_info_flag) {
      int display_width  = m_seqhdr.display_width;
      int display_height = m_seqhdr.display_height;

      set_video_display_dimensions(display_width, display_height, generic_packetizer_c::ddu_pixels, option_source_e::bitstream);

    } else
      set_video_display_dimensions(m_seqhdr.pixel_width, m_seqhdr.pixel_height, generic_packetizer_c::ddu_pixels, option_source_e::bitstream);

    if (m_default_duration_forced)
      m_parser.set_default_duration(get_track_default_duration());
    else
      set_track_default_duration(m_parser.get_default_duration());

    memcpy(((uint8_t *)bih) + sizeof(alBITMAPINFOHEADER) + 1, m_raw_headers->get_buffer(), m_raw_headers->get_size());

  } else
    set_track_default_duration(1000000000ll * 1001 / 30000);

  set_codec_private(buf);

  generic_packetizer_c::set_headers();

  m_track_entry->EnableLacing(false);
}

void
vc1_video_packetizer_c::add_timestamps_to_parser(packet_cptr const &packet) {
  if (-1 != packet->timestamp)
    m_parser.add_timestamp(packet->timestamp, 0);

  for (auto &extension : packet->extensions) {
    if (extension->get_type() != packet_extension_c::MULTIPLE_TIMESTAMPS)
      continue;

    multiple_timestamps_packet_extension_c *tc_extension = static_cast<multiple_timestamps_packet_extension_c *>(extension.get());
    int64_t timestamp, position;

    while (tc_extension->get_next(timestamp, position))
      m_parser.add_timestamp(timestamp, position);
  }
}

void
vc1_video_packetizer_c::process_impl(packet_cptr const &packet) {
  add_timestamps_to_parser(packet);

  m_parser.add_bytes(packet->data->get_buffer(), packet->data->get_size());

  if (!m_raw_headers && m_parser.are_headers_available())
    headers_found();

  flush_frames();
}

void
vc1_video_packetizer_c::headers_found() {
  m_parser.get_sequence_header(m_seqhdr);

  memory_cptr raw_seqhdr     = m_parser.get_raw_sequence_header();
  memory_cptr raw_entrypoint = m_parser.get_raw_entrypoint();

  m_raw_headers              = memory_c::alloc(raw_seqhdr->get_size() + raw_entrypoint->get_size());

  memcpy(m_raw_headers->get_buffer(),                          raw_seqhdr->get_buffer(),     raw_seqhdr->get_size());
  memcpy(m_raw_headers->get_buffer() + raw_seqhdr->get_size(), raw_entrypoint->get_buffer(), raw_entrypoint->get_size());

  if (!m_reader->m_appending) {
    set_headers();
    rerender_track_headers();
  }
}

void
vc1_video_packetizer_c::flush_impl() {
  m_parser.flush();
  flush_frames();
}

void
vc1_video_packetizer_c::flush_frames() {
  while (m_parser.is_frame_available()) {
    auto frame = m_parser.get_frame();
    add_packet(std::make_shared<packet_t>(frame->data, frame->timestamp, frame->duration, frame->is_key() ? -1 : m_previous_timestamp));

    m_previous_timestamp = frame->timestamp;
  }
}

connection_result_e
vc1_video_packetizer_c::can_connect_to(generic_packetizer_c *src,
                                       std::string &) {
  vc1_video_packetizer_c *vsrc = dynamic_cast<vc1_video_packetizer_c *>(src);
  if (!vsrc)
    return CAN_CONNECT_NO_FORMAT;

  return CAN_CONNECT_YES;
}
