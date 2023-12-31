/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   PCM output module

   Written by Moritz Bunkus <moritz@bunkus.org>.
   Patches by Robert Millan <rmh@aybabtu.com>.
*/

#include "common/common_pch.h"

#include "common/bswap.h"
#include "common/codec.h"
#include "common/list_utils.h"
#include "merge/connection_checks.h"
#include "merge/output_control.h"
#include "output/p_pcm.h"

pcm_packetizer_c::pcm_packetizer_c(generic_reader_c *p_reader,
                                   track_info_c &p_ti,
                                   int samples_per_sec,
                                   int channels,
                                   int bits_per_sample,
                                   pcm_format_e format)
  : generic_packetizer_c{p_reader, p_ti, track_audio}
  , m_samples_per_sec(samples_per_sec)
  , m_channels(channels)
  , m_bits_per_sample(bits_per_sample)
  , m_samples_per_packet{}
  , m_samples_per_packet_packaged{}
  , m_packet_size(0)
  , m_min_packet_size{}
  , m_samples_output(0)
  , m_num_durations_provided{}
  , m_num_packets_with_different_sample_count{}
  , m_format{format}
  , m_s2ts(1000000000ll, m_samples_per_sec)
{

  // make sure each packet will not be less than 4ms
  if ((samples_per_sec % 225) == 0)
    m_min_packet_size = samples_to_size(samples_per_sec / 225);
  else
    m_min_packet_size = samples_to_size(samples_per_sec / 250);

  // fall back to 40ms; it merely happens to be "second-aligned" for all common rates
  m_samples_per_packet = samples_per_sec / 25;
  m_packet_size        = samples_to_size(m_samples_per_packet);

  set_track_default_duration((int64_t)(1000000000.0 * m_samples_per_packet / m_samples_per_sec));

  if (m_format == big_endian_integer)
    m_byte_swapper = [this](uint8_t const *src, uint8_t *dst, std::size_t num_bytes) {
      mtx::bytes::swap_buffer(src, dst, num_bytes, m_bits_per_sample / 8);
    };
}

pcm_packetizer_c::~pcm_packetizer_c() {
}

void
pcm_packetizer_c::set_headers() {
  auto codec_id = ieee_float == m_format ? MKV_A_PCM_FLOAT : MKV_A_PCM;
  set_codec_id(codec_id);
  set_audio_sampling_freq(static_cast<double>(m_samples_per_sec));
  set_audio_channels(m_channels);
  set_audio_bit_depth(m_bits_per_sample);

  generic_packetizer_c::set_headers();
}

int64_t
pcm_packetizer_c::samples_to_size(int64_t samples)
  const {
  return (samples * m_channels * m_bits_per_sample) / 8;
}

int64_t
pcm_packetizer_c::size_to_samples(int64_t size)
  const {
  return (size * 8) / (m_channels * m_bits_per_sample);
}

void
pcm_packetizer_c::process_impl(packet_cptr const &packet) {
  if (packet->has_timestamp() && (packet->data->get_size() >= m_min_packet_size))
    return process_packaged(packet);

  m_buffer.add(packet->data->get_buffer(), packet->data->get_size());

  flush_packets();
}

void
pcm_packetizer_c::flush_packets() {
  while (m_buffer.get_size() >= m_packet_size) {
    auto packet = std::make_shared<packet_t>(memory_c::clone(m_buffer.get_buffer(), m_packet_size), m_samples_output * m_s2ts, m_samples_per_packet * m_s2ts);

    byte_swap_data(*packet->data);

    add_packet(packet);

    m_buffer.remove(m_packet_size);
    m_samples_output += m_samples_per_packet;
  }
}

void
pcm_packetizer_c::process_packaged(packet_cptr const &packet) {
  auto buffer_size = m_buffer.get_size();

  if (buffer_size) {
    auto data_size = packet->data->get_size();
    packet->data->resize(data_size + buffer_size);

    auto data = packet->data->get_buffer();

    std::memmove(&data[buffer_size], &data[0],              data_size);
    std::memmove(&data[0],           m_buffer.get_buffer(), buffer_size);

    m_buffer.remove(buffer_size);

    packet->timestamp -= size_to_samples(buffer_size) * m_s2ts;
  }

  auto samples_here = size_to_samples(packet->data->get_size());
  packet->duration  = samples_here * m_s2ts;
  m_samples_output  = packet->timestamp / m_s2ts + samples_here;

  ++m_num_durations_provided;

  if (1 == m_num_durations_provided) {
    m_samples_per_packet_packaged = samples_here;
    set_track_default_duration(samples_here * m_s2ts);
    rerender_track_headers();

  } else if (m_htrack_default_duration && (samples_here != m_samples_per_packet_packaged)) {
    ++m_num_packets_with_different_sample_count;

    if (1 < m_num_packets_with_different_sample_count) {
      set_track_default_duration(0);
      rerender_track_headers();
    }
  }

  byte_swap_data(*packet->data);

  add_packet(packet);
}

void
pcm_packetizer_c::flush_impl() {
  uint32_t size = m_buffer.get_size();
  if (0 >= size)
    return;

  int64_t samples_here = size_to_samples(size);
  auto packet          = std::make_shared<packet_t>(memory_c::clone(m_buffer.get_buffer(), size), m_samples_output * m_s2ts, samples_here * m_s2ts);

  byte_swap_data(*packet->data);

  add_packet(packet);

  m_samples_output += samples_here;
  m_buffer.remove(size);
}

void
pcm_packetizer_c::byte_swap_data(memory_c &data)
  const {
  if (!m_byte_swapper)
    return;

  data.resize(data.get_size() - (data.get_size() % (m_bits_per_sample / 8)));
  m_byte_swapper(data.get_buffer(), data.get_buffer(), data.get_size());
}

connection_result_e
pcm_packetizer_c::can_connect_to(generic_packetizer_c *src,
                                 std::string &error_message) {
  pcm_packetizer_c *psrc = dynamic_cast<pcm_packetizer_c *>(src);
  if (!psrc)
    return CAN_CONNECT_NO_FORMAT;

  connect_check_a_samplerate(m_samples_per_sec, psrc->m_samples_per_sec);
  connect_check_a_channels(m_channels, psrc->m_channels);
  connect_check_a_bitdepth(m_bits_per_sample, psrc->m_bits_per_sample);

  return CAN_CONNECT_YES;
}
