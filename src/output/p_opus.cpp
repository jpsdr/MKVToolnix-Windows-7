/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Opus packetizer

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/codec.h"
#include "common/opus.h"
#include "merge/connection_checks.h"
#include "output/p_opus.h"

opus_packetizer_c::opus_packetizer_c(generic_reader_c *reader,
                                     track_info_c &ti)
  : generic_packetizer_c{reader, ti, track_audio}
  , m_debug{"opus|opus_packetizer"}
  , m_next_calculated_timestamp{timestamp_c::ns(0)}
  , m_id_header(mtx::opus::id_header_t::decode(ti.m_private_data))
{
  mxdebug_if(m_debug, fmt::format("ID header: {0}\n", m_id_header));

  set_codec_private(m_ti.m_private_data);
}

opus_packetizer_c::~opus_packetizer_c() {
}

void
opus_packetizer_c::set_headers() {
  set_codec_id(fmt::format("{0}", MKV_A_OPUS));

  set_codec_delay(timestamp_c::samples(m_id_header.pre_skip, 48000));
  set_track_seek_pre_roll(timestamp_c::ms(80));

  set_audio_sampling_freq(m_id_header.input_sample_rate);
  set_audio_channels(m_id_header.channels);

  generic_packetizer_c::set_headers();
}

void
opus_packetizer_c::process_impl(packet_cptr const &packet) {
  try {
    auto toc = mtx::opus::toc_t::decode(packet->data);

    if (!packet->has_timestamp() || (timestamp_c::ns(packet->timestamp) == m_previous_provided_timestamp))
      packet->timestamp             = m_next_calculated_timestamp.to_ns();
    else
      m_previous_provided_timestamp = timestamp_c::ns(packet->timestamp);

    packet->duration                = toc.packet_duration.to_ns();
    m_next_calculated_timestamp     = timestamp_c::ns(packet->timestamp + packet->duration);

    mxdebug_if(m_debug, fmt::format("TOC: {0} discard_padding {1} final timestamp {2} duration {3}\n", toc, packet->discard_padding, mtx::string::format_timestamp(packet->timestamp), mtx::string::format_timestamp(packet->duration)));

    if (packet->discard_padding.valid())
      packet->duration_mandatory = true;

    add_packet(packet);

  } catch (mtx::opus::exception &ex) {
    mxdebug_if(m_debug, fmt::format("Exception: {0}\n", ex.what()));
  }
}

connection_result_e
opus_packetizer_c::can_connect_to(generic_packetizer_c *src,
                                    std::string &error_message) {
  opus_packetizer_c *vsrc = dynamic_cast<opus_packetizer_c *>(src);
  if (!vsrc)
    return CAN_CONNECT_NO_FORMAT;

  connect_check_a_samplerate(m_id_header.input_sample_rate, vsrc->m_id_header.input_sample_rate);
  connect_check_a_channels(m_id_header.channels,            vsrc->m_id_header.channels);
  connect_check_codec_private(vsrc);

  return CAN_CONNECT_YES;
}

bool
opus_packetizer_c::is_compatible_with(output_compatibility_e compatibility) {
  // TODO: check for WebM compatibility
  return (OC_MATROSKA == compatibility) || (OC_WEBM == compatibility);
}
