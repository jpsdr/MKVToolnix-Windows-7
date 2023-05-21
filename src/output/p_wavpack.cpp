/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   WAVPACK output module

   Written by Steve Lhomme <steve.lhomme@free.fr>.
*/

#include "common/common_pch.h"

#include <cmath>

#include <matroska/KaxTracks.h>

#include "common/codec.h"
#include "common/endian.h"
#include "merge/connection_checks.h"
#include "output/p_wavpack.h"

using namespace libmatroska;

wavpack_packetizer_c::wavpack_packetizer_c(generic_reader_c *p_reader,
                                           track_info_c &p_ti,
                                           mtx::wavpack::meta_t &meta)
  : generic_packetizer_c{p_reader, p_ti, track_audio}
  , m_channels(meta.channel_count)
  , m_sample_rate(meta.sample_rate)
  , m_bits_per_sample(meta.bits_per_sample)
  , m_samples_per_block(meta.samples_per_block)
  , m_samples_output(0)
  , m_has_correction(meta.has_correction)
{
  m_sample_duration = mtx::rational(1'000'000'000, m_sample_rate);
}

void
wavpack_packetizer_c::set_headers() {
  set_codec_id(MKV_A_WAVPACK4);
  set_codec_private(m_ti.m_private_data);
  set_audio_sampling_freq(m_sample_rate);
  set_audio_channels(m_channels);
  set_audio_bit_depth(m_bits_per_sample);
  set_track_default_duration(m_samples_per_block * 1000000000 / m_sample_rate);

  generic_packetizer_c::set_headers();

  m_track_entry->EnableLacing(!m_has_correction);
}

void
wavpack_packetizer_c::process_impl(packet_cptr const &packet) {
  int64_t samples = get_uint32_le(packet->data->get_buffer());

  if (-1 == packet->duration)
    packet->duration = mtx::to_int_rounded(samples * m_sample_duration);

  if (-1 == packet->timestamp)
    packet->timestamp = mtx::to_int_rounded(m_samples_output * m_sample_duration);

  m_samples_output += samples;
  add_packet(packet);
}

connection_result_e
wavpack_packetizer_c::can_connect_to(generic_packetizer_c *src,
                                     std::string &error_message) {
  wavpack_packetizer_c *psrc = dynamic_cast<wavpack_packetizer_c *>(src);
  if (!psrc)
    return CAN_CONNECT_NO_FORMAT;

  connect_check_a_samplerate(m_sample_rate,   psrc->m_sample_rate);
  connect_check_a_channels(m_channels,        psrc->m_channels);
  connect_check_a_bitdepth(m_bits_per_sample, psrc->m_bits_per_sample);

  return CAN_CONNECT_YES;
}
