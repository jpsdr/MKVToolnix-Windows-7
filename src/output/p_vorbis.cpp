/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Vorbis packetizer

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <ogg/ogg.h>
#include <vorbis/codec.h>

#include "common/codec.h"
#include "merge/connection_checks.h"
#include "merge/output_control.h"
#include "output/p_vorbis.h"

vorbis_packetizer_c::vorbis_packetizer_c(generic_reader_c *reader,
                                         track_info_c &ti,
                                         std::vector<memory_cptr> const &headers)
  : generic_packetizer_c{reader, ti, track_audio}
  , m_headers{headers}
{
  for (auto const &header : m_headers)
    header->take_ownership();

  ogg_packet ogg_headers[3];
  memset(ogg_headers, 0, 3 * sizeof(ogg_packet));

  for (auto idx = 0; idx < 3; ++idx) {
    ogg_headers[idx].packet   = m_headers[idx]->get_buffer();
    ogg_headers[idx].bytes    = m_headers[idx]->get_size();
  }

  ogg_headers[0].b_o_s    = 1;
  ogg_headers[1].packetno = 1;
  ogg_headers[2].packetno = 2;

  vorbis_info_init(&m_vi);
  vorbis_comment_init(&m_vc);

  int i;
  for (i = 0; 3 > i; ++i)
    if (vorbis_synthesis_headerin(&m_vi, &m_vc, &ogg_headers[i]) < 0)
      throw mtx::output::vorbis_x(Y("Error: vorbis_packetizer: Could not extract the stream's parameters from the first packets.\n"));

  if (g_use_durations)
    set_track_default_duration((int64_t)(1024000000000.0 / m_vi.rate));
}

vorbis_packetizer_c::~vorbis_packetizer_c() {
  vorbis_info_clear(&m_vi);
  vorbis_comment_clear(&m_vc);
}

void
vorbis_packetizer_c::set_headers() {

  set_codec_id(MKV_A_VORBIS);

  set_codec_private(lace_memory_xiph(m_headers));

  set_audio_sampling_freq(m_vi.rate);
  set_audio_channels(m_vi.channels);

  generic_packetizer_c::set_headers();
}

void
vorbis_packetizer_c::process_impl(packet_cptr const &packet) {
  ogg_packet op;

  // Remember the very first timestamp we received.
  if ((0 == m_samples) && (0 < packet->timestamp))
    m_timestamp_offset = packet->timestamp;

  // Update the number of samples we have processed so that we can
  // calculate the timestamp on the next call.
  op.packet                  = packet->data->get_buffer();
  op.bytes                   = packet->data->get_size();
  int64_t this_bs            = vorbis_packet_blocksize(&m_vi, &op);
  int64_t samples_here       = (this_bs + m_previous_bs) / 4;
  m_previous_bs              = this_bs;
  m_samples                 += samples_here;

  int64_t expected_timestamp = m_previous_timestamp + m_previous_samples_sum * 1000000000 / m_vi.rate + m_timestamp_offset;
  int64_t chosen_timestamp;

  if (packet->timestamp > (expected_timestamp + 100000000)) {
    chosen_timestamp       = packet->timestamp;
    packet->duration       = packet->timestamp - (m_previous_timestamp + m_previous_samples_sum * 1000000000 / m_vi.rate + m_timestamp_offset);
    m_previous_timestamp   = packet->timestamp;
    m_previous_samples_sum = 0;

  } else {
    chosen_timestamp = expected_timestamp;
    packet->duration = (int64_t)(samples_here * 1000000000 / m_vi.rate);
  }

  m_previous_samples_sum += samples_here;
  packet->timestamp       = chosen_timestamp;

  add_packet(packet);
}

connection_result_e
vorbis_packetizer_c::can_connect_to(generic_packetizer_c *src,
                                    std::string &error_message) {
  vorbis_packetizer_c *vsrc = dynamic_cast<vorbis_packetizer_c *>(src);
  if (!vsrc)
    return CAN_CONNECT_NO_FORMAT;

  connect_check_a_samplerate(m_vi.rate,   vsrc->m_vi.rate);
  connect_check_a_channels(m_vi.channels, vsrc->m_vi.channels);

  if ((m_headers[2]->get_size() != vsrc->m_headers[2]->get_size()) || memcmp(m_headers[2]->get_buffer(), vsrc->m_headers[2]->get_buffer(), m_headers[2]->get_size())) {
    error_message = Y("The Vorbis codebooks are different. Such tracks cannot be concatenated without re-encoding.");
    return CAN_CONNECT_NO_FORMAT;
  }

  return CAN_CONNECT_YES;
}

bool
vorbis_packetizer_c::is_compatible_with(output_compatibility_e compatibility) {
  return (OC_MATROSKA == compatibility) || (OC_WEBM == compatibility);
}
