/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   TTA output module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <cmath>

#include "common/codec.h"
#include "common/tta.h"
#include "merge/connection_checks.h"
#include "output/p_tta.h"

tta_packetizer_c::tta_packetizer_c(generic_reader_c *p_reader,
                                   track_info_c &p_ti,
                                   int channels,
                                   int bits_per_sample,
                                   int sample_rate)
  : generic_packetizer_c{p_reader, p_ti, track_audio}
  , m_channels(channels)
  , m_bits_per_sample(bits_per_sample)
  , m_sample_rate(sample_rate)
  , m_samples_output(0)
{
  set_track_default_duration(std::llround(1000000000.0 * mtx::tta::FRAME_TIME));

  m_samples_per_frame = std::llround(m_sample_rate * mtx::tta::FRAME_TIME);
}

tta_packetizer_c::~tta_packetizer_c() {
}

void
tta_packetizer_c::set_headers() {
  set_codec_id(MKV_A_TTA);
  set_audio_sampling_freq(m_sample_rate);
  set_audio_channels(m_channels);
  set_audio_bit_depth(m_bits_per_sample);

  generic_packetizer_c::set_headers();
}

void
tta_packetizer_c::process_impl(packet_cptr const &packet) {
  packet->timestamp = mtx::to_int(mtx::rational(m_samples_output, m_sample_rate) * 1'000'000'000);
  if (-1 == packet->duration) {
    packet->duration  = m_htrack_default_duration;
    m_samples_output += m_samples_per_frame;

  } else
    m_samples_output += mtx::to_int(mtx::rational(packet->duration, 1'000'000'000) * m_sample_rate);

  add_packet(packet);
}

connection_result_e
tta_packetizer_c::can_connect_to(generic_packetizer_c *src,
                                 std::string &error_message) {
  tta_packetizer_c *psrc = dynamic_cast<tta_packetizer_c *>(src);
  if (!psrc)
    return CAN_CONNECT_NO_FORMAT;

  connect_check_a_samplerate(m_sample_rate,   psrc->m_sample_rate);
  connect_check_a_channels(m_channels,        psrc->m_channels);
  connect_check_a_bitdepth(m_bits_per_sample, psrc->m_bits_per_sample);

  return CAN_CONNECT_YES;
}
