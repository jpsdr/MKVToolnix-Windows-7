/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   WAV PCM demuxer module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "avilib.h"
#include "common/endian.h"
#include "input/r_wav.h"
#include "input/wav_pcm_demuxer.h"
#include "output/p_pcm.h"

wav_pcm_demuxer_c::wav_pcm_demuxer_c(wav_reader_c *reader,
                                     wave_header  *wheader,
                                     bool ieee_float)
  : wav_demuxer_c{reader, wheader}
  , m_bps{}
  , m_ieee_float{ieee_float}
{
  m_bps    = get_channels() * get_bits_per_sample() * get_sampling_frequency() / 8;
  m_buffer = memory_c::alloc(m_bps);

  m_codec  = codec_c::look_up(codec_c::type_e::A_PCM);
}

wav_pcm_demuxer_c::~wav_pcm_demuxer_c() {
}

generic_packetizer_c *
wav_pcm_demuxer_c::create_packetizer() {
  m_ptzr = new pcm_packetizer_c(m_reader, m_ti, get_sampling_frequency(), get_channels(), get_bits_per_sample(), m_ieee_float ? pcm_packetizer_c::ieee_float : pcm_packetizer_c::little_endian_integer);

  m_reader->show_packetizer_info(0, *m_ptzr);

  return m_ptzr;
}

void
wav_pcm_demuxer_c::process(int64_t len) {
  if (0 >= len)
    return;

  m_ptzr->process(std::make_shared<packet_t>(memory_c::borrow(m_buffer->get_buffer(), len)));
}

unsigned int
wav_pcm_demuxer_c::get_channels()
  const {
  return get_uint16_le(&m_wheader->common.wChannels);
}

unsigned int
wav_pcm_demuxer_c::get_sampling_frequency()
  const {
  return get_uint32_le(&m_wheader->common.dwSamplesPerSec);
}

unsigned int
wav_pcm_demuxer_c::get_bits_per_sample()
  const {
  return get_uint16_le(&m_wheader->common.wBitsPerSample);
}
