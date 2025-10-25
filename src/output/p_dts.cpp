/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   DTS output module

   Written by Peter Niemayer <niemayer@isg.de>.
   Modified by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/codec.h"
#include "common/dts.h"
#include "merge/connection_checks.h"
#include "merge/output_control.h"
#include "output/p_dts.h"
#include "common/strings/formatting.h"

dts_packetizer_c::dts_packetizer_c(generic_reader_c *p_reader,
                                   track_info_c &p_ti,
                                   mtx::dts::header_t const &dtsheader)
  : generic_packetizer_c{p_reader, p_ti, track_audio}
  , m_packet_buffer{128 * 1024}
  , m_first_header{dtsheader}
  , m_previous_header{dtsheader}
  , m_skipping_is_normal{}
  , m_reduce_to_core{get_option_for_track(m_ti.m_reduce_to_core, m_ti.m_id)}
  , m_remove_dialog_normalization_gain{get_option_for_track(m_ti.m_remove_dialog_normalization_gain, m_ti.m_id)}
  , m_timestamp_calculator{static_cast<int64_t>(m_first_header.core_sampling_frequency)}
  , m_stream_position{}
  , m_packet_position{}
{
}

dts_packetizer_c::~dts_packetizer_c() {
}

dts_packetizer_c::header_and_packet_t
dts_packetizer_c::get_dts_packet(bool flushing) {
  mtx::dts::header_t dtsheader{};
  memory_cptr packet_buf;

  if (0 == m_packet_buffer.get_size())
    return std::make_tuple(dtsheader, packet_buf, 0ull);

  const uint8_t *buf = m_packet_buffer.get_buffer();
  int buf_size             = m_packet_buffer.get_size();
  int pos                  = mtx::dts::find_sync_word(buf, buf_size);

  if (0 > pos) {
    if (4 < buf_size)
      m_packet_buffer.remove(buf_size - 4);
    return std::make_tuple(dtsheader, packet_buf, 0ull);
  }

  if (0 < pos) {
    m_packet_buffer.remove(pos);
    buf      = m_packet_buffer.get_buffer();
    buf_size = m_packet_buffer.get_size();
  }

  pos = mtx::dts::find_header(buf, buf_size, dtsheader, flushing);

  if ((0 > pos) || (static_cast<int>(pos + dtsheader.frame_byte_size) > buf_size))
    return std::make_tuple(dtsheader, packet_buf, 0ull);

  if ((1 < verbose) && (dtsheader != m_previous_header)) {
    mxinfo(Y("DTS header information changed! - New format:\n"));
    dtsheader.print();
    m_previous_header = dtsheader;
  }

  if (verbose && (0 < pos) && !m_skipping_is_normal) {
    int i;
    bool all_zeroes = true;

    for (i = 0; i < pos; ++i)
      if (buf[i]) {
        all_zeroes = false;
        break;
      }

    if (!all_zeroes)
      mxwarn_tid(m_ti.m_fname, m_ti.m_id, fmt::format(FY("Skipping {0} bytes (no valid DTS header found). This might cause audio/video desynchronisation.\n"), pos));
  }

  auto bytes_to_remove = pos + dtsheader.frame_byte_size;
  auto packet_position = m_packet_position + pos;
  m_packet_position   += bytes_to_remove;

  if (   m_reduce_to_core
      && dtsheader.has_core
      && dtsheader.has_exss
      && (dtsheader.exss_part_size > 0)
      && (dtsheader.exss_part_size < dtsheader.frame_byte_size)) {
    dtsheader.frame_byte_size -= dtsheader.exss_part_size;
    dtsheader.has_exss         = false;
  }

  packet_buf = memory_c::clone(buf + pos, dtsheader.frame_byte_size);

  m_packet_buffer.remove(bytes_to_remove);

  return std::make_tuple(dtsheader, packet_buf, packet_position);
}

void
dts_packetizer_c::set_headers() {
  set_codec_id(MKV_A_DTS);
  set_audio_sampling_freq(m_first_header.get_effective_sampling_frequency());
  set_audio_channels(m_reduce_to_core ? m_first_header.get_core_num_audio_channels() : m_first_header.get_total_num_audio_channels());
  set_track_default_duration(m_first_header.get_packet_length_in_nanoseconds().to_ns());
  if (m_first_header.source_pcm_resolution > 0)
    set_audio_bit_depth(m_first_header.source_pcm_resolution);

  generic_packetizer_c::set_headers();
}

void
dts_packetizer_c::process_impl(packet_cptr const &packet) {
  m_timestamp_calculator.add_timestamp(packet, m_stream_position);
  m_discard_padding.add_maybe(packet->discard_padding, m_stream_position);
  m_stream_position += packet->data->get_size();

  m_packet_buffer.add(packet->data->get_buffer(), packet->data->get_size());

  queue_available_packets(false);
  process_available_packets();
}

void
dts_packetizer_c::queue_available_packets(bool flushing) {
  while (true) {
    auto result = get_dts_packet(flushing);
    if (!std::get<1>(result))
      break;

    m_queued_packets.push_back(result);

    auto current_sampling_frequency = std::get<0>(result).core_sampling_frequency;

    if (!m_first_header.core_sampling_frequency && current_sampling_frequency) {
      m_first_header.core_sampling_frequency = current_sampling_frequency;
      m_timestamp_calculator                 = timestamp_calculator_c{static_cast<int64_t>(current_sampling_frequency)};

      set_audio_sampling_freq(m_first_header.get_effective_sampling_frequency());

      rerender_track_headers();
    }
  }
}

void
dts_packetizer_c::process_available_packets() {
  if (!m_first_header.core_sampling_frequency)
    return;

  for (auto const &header_and_packet : m_queued_packets) {
    auto &header            = std::get<0>(header_and_packet);
    auto &data              = std::get<1>(header_and_packet);
    auto packet_position    = std::get<2>(header_and_packet);
    auto samples_in_packet  = header.get_packet_length_in_core_samples();
    auto new_timestamp      = m_timestamp_calculator.get_next_timestamp(samples_in_packet, packet_position);
    auto packet             = std::make_shared<packet_t>(data, new_timestamp.to_ns(), header.get_packet_length_in_nanoseconds().to_ns());
    packet->discard_padding = m_discard_padding.get_next(packet_position).value_or(timestamp_c{});

    if (m_remove_dialog_normalization_gain)
      mtx::dts::remove_dialog_normalization_gain(data->get_buffer(), data->get_size());

    add_packet(packet);
  }

  m_queued_packets.clear();
}

void
dts_packetizer_c::flush_impl() {
  queue_available_packets(true);
  process_available_packets();
}

connection_result_e
dts_packetizer_c::can_connect_to(generic_packetizer_c *src,
                                 std::string &error_message) {
  dts_packetizer_c *dsrc = dynamic_cast<dts_packetizer_c *>(src);
  if (!dsrc)
    return CAN_CONNECT_NO_FORMAT;

  connect_check_a_samplerate(m_first_header.core_sampling_frequency, dsrc->m_first_header.core_sampling_frequency);
  connect_check_a_channels(m_first_header.audio_channels, dsrc->m_first_header.audio_channels);

  return CAN_CONNECT_YES;
}
