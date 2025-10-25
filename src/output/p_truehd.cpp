/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   TRUEHD output module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <matroska/KaxTracks.h>

#include "common/codec.h"
#include "common/truehd.h"
#include "merge/connection_checks.h"
#include "merge/output_control.h"
#include "output/p_truehd.h"

truehd_packetizer_c::truehd_packetizer_c(generic_reader_c *p_reader,
                                         track_info_c &p_ti,
                                         mtx::truehd::frame_t::codec_e codec,
                                         int sampling_rate,
                                         int channels)
  : generic_packetizer_c{p_reader, p_ti, track_audio}
  , m_first_frame{true}
  , m_remove_dialog_normalization_gain{get_option_for_track(m_ti.m_remove_dialog_normalization_gain, m_ti.m_id)}
  , m_current_samples_per_frame{}
  , m_ref_timestamp{}
  , m_timestamp_calculator{sampling_rate}
{
  m_first_truehd_header.m_codec         = codec;
  m_first_truehd_header.m_sampling_rate = sampling_rate;
  m_first_truehd_header.m_channels      = channels;
}

truehd_packetizer_c::~truehd_packetizer_c() {
}

void
truehd_packetizer_c::set_headers() {
  set_codec_id(m_first_truehd_header.is_truehd() ? MKV_A_TRUEHD : MKV_A_MLP);
  set_audio_sampling_freq(m_first_truehd_header.m_sampling_rate);
  set_audio_channels(m_first_truehd_header.m_channels);

  generic_packetizer_c::set_headers();
}

void
truehd_packetizer_c::process_framed(mtx::truehd::frame_cptr const &frame,
                                    std::optional<int64_t> provided_timestamp) {
  m_timestamp_calculator.add_timestamp(provided_timestamp);

  if (frame->is_ac3())
    return;

  if (frame->is_sync()) {
    adjust_header_values(frame);
    m_current_samples_per_frame = frame->m_samples_per_frame;
  }

  auto samples            = 0 == frame->m_samples_per_frame ? m_current_samples_per_frame : frame->m_samples_per_frame;
  auto timestamp          = m_timestamp_calculator.get_next_timestamp(samples).to_ns();
  auto duration           = m_timestamp_calculator.get_duration(samples).to_ns();
  auto packet             = std::make_shared<packet_t>(frame->m_data, timestamp, duration, frame->is_sync() ? -1 : m_ref_timestamp);
  packet->discard_padding = m_discard_padding.get_next().value_or(timestamp_c{});

  if (frame->is_sync() && frame->is_truehd() && m_remove_dialog_normalization_gain)
    mtx::truehd::remove_dialog_normalization_gain(frame->m_data->get_buffer(), frame->m_data->get_size());

  add_packet(packet);

  m_ref_timestamp = timestamp;
}

void
truehd_packetizer_c::process_impl(packet_cptr const &packet) {
  m_timestamp_calculator.add_timestamp(packet);
  m_discard_padding.add_maybe(packet->discard_padding);

  m_parser.add_data(packet->data->get_buffer(), packet->data->get_size());

  flush_frames();
}

void
truehd_packetizer_c::flush_frames() {
  while (m_parser.frame_available())
    process_framed(m_parser.get_next_frame());
}

void
truehd_packetizer_c::adjust_header_values(mtx::truehd::frame_cptr const &frame) {
  if (!m_first_frame)
    return;

  bool rerender_headers = false;
  if (frame->m_codec != m_first_truehd_header.m_codec) {
    rerender_headers              = true;
    m_first_truehd_header.m_codec = frame->m_codec;
    set_codec_id(m_first_truehd_header.is_truehd() ? MKV_A_TRUEHD : MKV_A_MLP);
  }

  if (frame->m_channels != m_first_truehd_header.m_channels) {
    rerender_headers                 = true;
    m_first_truehd_header.m_channels = frame->m_channels;
    set_audio_channels(m_first_truehd_header.m_channels);
  }

  if (frame->m_sampling_rate != m_first_truehd_header.m_sampling_rate) {
    rerender_headers                      = true;
    m_first_truehd_header.m_sampling_rate = frame->m_sampling_rate;
    set_audio_channels(m_first_truehd_header.m_sampling_rate);
  }

  m_first_truehd_header.m_samples_per_frame = frame->m_samples_per_frame;

  if (rerender_headers)
    rerender_track_headers();

  m_first_frame = false;

  m_timestamp_calculator.set_samples_per_second(m_first_truehd_header.m_sampling_rate);
}

void
truehd_packetizer_c::flush_impl() {
  m_parser.parse(true);
  flush_frames();
}

connection_result_e
truehd_packetizer_c::can_connect_to(generic_packetizer_c *src,
                                    std::string &error_message) {
  truehd_packetizer_c *asrc = dynamic_cast<truehd_packetizer_c *>(src);
  if (!asrc)
    return CAN_CONNECT_NO_FORMAT;

  connect_check_a_samplerate(m_first_truehd_header.m_sampling_rate, asrc->m_first_truehd_header.m_sampling_rate);
  connect_check_a_channels(m_first_truehd_header.m_channels, asrc->m_first_truehd_header.m_channels);

  return CAN_CONNECT_YES;
}
