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
#include "common/strings/formatting.h"
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
  , m_packet_size{}
  , m_samples_output(0)
  , m_format{format}
  , m_s2ts(1000000000ll, m_samples_per_sec)
{
  // 40ms per packet; this happens to be "second-aligned" for all common rates
  m_samples_per_packet = samples_per_sec / 25;
  m_packet_size        = samples_to_size(m_samples_per_packet);

  mxdebug_if(m_debug, fmt::format("PCM packetizer ctor: samples_per_sec {0} samples_per_packet {1} packet_size {2} channels {3} bits_per_sample {4}\n", samples_per_sec, m_samples_per_packet, m_packet_size, m_channels, m_bits_per_sample));

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
  auto buffered_size      = m_buffer.get_size();
  auto expected_timestamp = (m_samples_output + size_to_samples(buffered_size)) * m_s2ts;
  auto ts_diff            = packet->timestamp - expected_timestamp;

  if (packet->has_timestamp() && (std::abs(ts_diff) > 1'000'000)) {
    // Next data has timestamp other than one we expect (e.g. a gap is
    // following, or offset at the start of the track). Flush
    // currently buffered data (if any) with old calculated timestamp,
    // then re-derive number of samples output to provided timestamp
    // so that new timestamps will be calculated from the basis of the
    // incoming packet's timestamp.

    mxdebug_if(m_debug, fmt::format("PCM packetizer process: have gap of {0} in expected ({1}) vs provided ({2}) timestamps; flushing {3} bytes of buffered data = {4} samples = {5}\n",
                                    mtx::string::format_timestamp(ts_diff), mtx::string::format_timestamp(expected_timestamp), mtx::string::format_timestamp(packet->timestamp),
                                    buffered_size, size_to_samples(buffered_size), mtx::string::format_timestamp(size_to_samples(buffered_size) * m_s2ts)));

    flush_impl();
    m_samples_output = packet->timestamp / m_s2ts;
  }

  if (!packet->data->get_size())
    return;

  m_buffer.add(*packet->data);

  auto size_to_flush_before = m_buffer.get_size();
  auto old_samples_output   = m_samples_output;

  flush_packets();

  auto size_flushed = size_to_flush_before - m_buffer.get_size();

  mxdebug_if(m_debug, fmt::format("PCM packetizer process: old TS {0} new TS {1} added {2} flushed {3} ({4}) remaining {5}\n",
                                  mtx::string::format_timestamp(old_samples_output * m_s2ts), mtx::string::format_timestamp(m_samples_output * m_s2ts),
                                  packet->data->get_size(), size_flushed, size_flushed / m_packet_size, m_buffer.get_size()));
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
pcm_packetizer_c::flush_impl() {
  uint32_t size = m_buffer.get_size();
  if (0 >= size)
    return;

  int64_t samples_here = size_to_samples(size);
  auto packet          = std::make_shared<packet_t>(memory_c::clone(m_buffer.get_buffer(), size), m_samples_output * m_s2ts, samples_here * m_s2ts);

  byte_swap_data(*packet->data);

  add_packet(packet);

  mxdebug_if(m_debug, fmt::format("PCM packetizer flush: TS {0} remaining {1} duration {2}\n", mtx::string::format_timestamp(m_samples_output * m_s2ts), m_buffer.get_size(),
                                  mtx::string::format_timestamp(m_buffer.get_size() * m_s2ts)));

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
