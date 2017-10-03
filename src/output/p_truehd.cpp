/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   TRUEHD output module

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <matroska/KaxTracks.h>

#include "common/codec.h"
#include "common/truehd.h"
#include "merge/connection_checks.h"
#include "merge/output_control.h"
#include "output/p_truehd.h"

using namespace libmatroska;

truehd_packetizer_c::truehd_packetizer_c(generic_reader_c *p_reader,
                                         track_info_c &p_ti,
                                         truehd_frame_t::codec_e codec,
                                         int sampling_rate,
                                         int channels)
  : generic_packetizer_c{p_reader, p_ti}
  , m_first_frame{true}
  , m_current_samples_per_frame{}
  , m_ref_timestamp{}
  , m_timestamp_calculator{sampling_rate}
{
  m_first_truehd_header.m_codec         = codec;
  m_first_truehd_header.m_sampling_rate = sampling_rate;
  m_first_truehd_header.m_channels      = channels;

  set_track_type(track_audio);
}

truehd_packetizer_c::~truehd_packetizer_c() {
}

void
truehd_packetizer_c::set_headers() {
  set_codec_id(m_first_truehd_header.is_truehd() ? MKV_A_TRUEHD : MKV_A_MLP);
  set_audio_sampling_freq((float)m_first_truehd_header.m_sampling_rate);
  set_audio_channels(m_first_truehd_header.m_channels);

  generic_packetizer_c::set_headers();
}

void
truehd_packetizer_c::process_framed(truehd_frame_cptr const &frame,
                                    int64_t provided_timestamp) {
  m_timestamp_calculator.add_timestamp(provided_timestamp);

  if (frame->is_ac3())
    return;

  if (frame->is_sync()) {
    adjust_header_values(frame);
    m_current_samples_per_frame = frame->m_samples_per_frame;
  }

  auto samples   = 0 == frame->m_samples_per_frame ? m_current_samples_per_frame : frame->m_samples_per_frame;
  auto timestamp = m_timestamp_calculator.get_next_timestamp(samples).to_ns();
  auto duration  = m_timestamp_calculator.get_duration(samples).to_ns();

  add_packet(std::make_shared<packet_t>(frame->m_data, timestamp, duration, frame->is_sync() ? -1 : m_ref_timestamp));

  m_ref_timestamp = timestamp;
}

int
truehd_packetizer_c::process(packet_cptr packet) {
  m_timestamp_calculator.add_timestamp(packet);

  m_parser.add_data(packet->data->get_buffer(), packet->data->get_size());

  flush_frames();

  return FILE_STATUS_MOREDATA;
}

void
truehd_packetizer_c::flush_frames() {
  while (m_parser.frame_available())
    process_framed(m_parser.get_next_frame(), -1);
}

void
truehd_packetizer_c::adjust_header_values(truehd_frame_cptr const &frame) {
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
