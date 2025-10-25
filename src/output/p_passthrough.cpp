/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   PASSTHROUGH output module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "merge/connection_checks.h"
#include "output/p_passthrough.h"

passthrough_packetizer_c::passthrough_packetizer_c(generic_reader_c *p_reader,
                                                   track_info_c &p_ti,
                                                   track_type type)
  : generic_packetizer_c{p_reader, p_ti, type}
{
  m_timestamp_factory_application_mode = TFA_FULL_QUEUEING;
}

void
passthrough_packetizer_c::set_headers() {
  generic_packetizer_c::set_headers();
}

void
passthrough_packetizer_c::process_impl(packet_cptr const &packet) {
  add_packet(packet);
}

connection_result_e
passthrough_packetizer_c::can_connect_to(generic_packetizer_c *src,
                                         std::string &error_message) {
  passthrough_packetizer_c *psrc = dynamic_cast<passthrough_packetizer_c *>(src);
  if (!psrc)
    return CAN_CONNECT_NO_FORMAT;

  connect_check_codec_id(m_hcodec_id, psrc->m_hcodec_id);

  if ((m_htrack_type != psrc->m_htrack_type) || (m_hcodec_id != psrc->m_hcodec_id))
    return CAN_CONNECT_NO_PARAMETERS;

  connect_check_codec_private(src);

  switch (m_htrack_type) {
    case track_video:
      connect_check_v_width(m_hvideo_pixel_width,      psrc->m_hvideo_pixel_width);
      connect_check_v_height(m_hvideo_pixel_height,    psrc->m_hvideo_pixel_height);
      connect_check_v_dwidth(m_hvideo_display_width,   psrc->m_hvideo_display_width);
      connect_check_v_dheight(m_hvideo_display_height, psrc->m_hvideo_display_height);
      if (m_htrack_default_duration != psrc->m_htrack_default_duration)
        return CAN_CONNECT_NO_PARAMETERS;
      break;

    case track_audio:
      connect_check_a_samplerate(m_haudio_sampling_freq, psrc->m_haudio_sampling_freq);
      connect_check_a_channels(m_haudio_channels,        psrc->m_haudio_channels);
      connect_check_a_bitdepth(m_haudio_bit_depth,       psrc->m_haudio_bit_depth);
      if (m_htrack_default_duration != psrc->m_htrack_default_duration)
        return CAN_CONNECT_NO_PARAMETERS;
      break;

    case track_subtitle:
      break;

    default:
      break;
  }

  return CAN_CONNECT_YES;
}
