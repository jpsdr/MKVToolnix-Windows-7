/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   AAC output module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/aac.h"
#include "common/codec.h"
#include "merge/connection_checks.h"
#include "merge/output_control.h"
#include "output/p_aac.h"

aac_packetizer_c::aac_packetizer_c(generic_reader_c *p_reader,
                                   track_info_c &p_ti,
                                   mtx::aac::audio_config_t const &config,
                                   mode_e mode)
  : generic_packetizer_c{p_reader, p_ti, track_audio}
  , m_config{config}
  , m_mode{mode}
  , m_timestamp_calculator{static_cast<int64_t>(config.sample_rate)}
  , m_packet_duration{}
  , m_first_packet{true}
{
  if (!m_config.samples_per_frame)
    m_config.samples_per_frame = 1024;

  m_packet_duration = m_timestamp_calculator.get_duration(m_config.samples_per_frame).to_ns();
}

aac_packetizer_c::~aac_packetizer_c() {
}

void
aac_packetizer_c::handle_parsed_audio_config() {
  if (!m_first_packet)
    return;

  m_first_packet  = false;
  auto raw_config = m_parser.get_audio_specific_config();

  if (!raw_config)
    return;

  auto parsed_config = mtx::aac::parse_audio_specific_config(raw_config->get_buffer(), raw_config->get_size());

  if (!parsed_config || (parsed_config->samples_per_frame == m_config.samples_per_frame))
    return;

  m_config.samples_per_frame = parsed_config->samples_per_frame;
  m_ti.m_private_data        = raw_config;
  m_packet_duration          = m_timestamp_calculator.get_duration(m_config.samples_per_frame).to_ns();

  set_headers();

  rerender_track_headers();
}

void
aac_packetizer_c::set_headers() {
  set_codec_id(MKV_A_AAC);
  set_audio_sampling_freq(m_config.sample_rate);
  set_audio_channels(m_config.channels);
  set_track_default_duration(m_packet_duration);

  if (m_config.sbr || (m_config.profile == mtx::aac::PROFILE_SBR)) {
    m_config.profile            = mtx::aac::PROFILE_LC;
    m_config.sbr                = true;
    m_config.output_sample_rate = std::max<unsigned int>(m_config.sample_rate * 2, m_config.output_sample_rate);

    set_audio_output_sampling_freq(m_config.output_sample_rate);
  }

  if (m_ti.m_private_data && (0 < m_ti.m_private_data->get_size()))
    set_codec_private(m_ti.m_private_data);

  else
    set_codec_private(mtx::aac::create_audio_specific_config(m_config));

  generic_packetizer_c::set_headers();
}

void
aac_packetizer_c::process_headerless(packet_cptr const &packet) {
  handle_parsed_audio_config();

  packet->timestamp       = m_timestamp_calculator.get_next_timestamp(m_config.samples_per_frame).to_ns();
  packet->duration        = m_packet_duration;
  packet->discard_padding = m_discard_padding.get_next().value_or(timestamp_c{});

  add_packet(packet);
}

void
aac_packetizer_c::process_impl(packet_cptr const &packet) {
  m_timestamp_calculator.add_timestamp(packet);
  m_discard_padding.add_maybe(packet->discard_padding);

  if (m_mode == mode_e::headerless) {
    process_headerless(packet);
    return;
  }

  m_parser.add_bytes(packet->data);

  if (!m_parser.headers_parsed())
    return;

  while (m_parser.frames_available()) {
    auto frame = m_parser.get_frame();

    process_headerless(std::make_shared<packet_t>(frame.m_data));

    if (verbose && frame.m_garbage_size)
      mxwarn_tid(m_ti.m_fname, m_ti.m_id, fmt::format(FY("Skipping {0} bytes (no valid AAC header found). This might cause audio/video desynchronisation.\n"), frame.m_garbage_size));
  }
}

connection_result_e
aac_packetizer_c::can_connect_to(generic_packetizer_c *src,
                                 std::string &error_message) {
  aac_packetizer_c *asrc = dynamic_cast<aac_packetizer_c *>(src);
  if (!asrc)
    return CAN_CONNECT_NO_FORMAT;

  connect_check_a_samplerate(m_config.sample_rate, asrc->m_config.sample_rate);
  connect_check_a_channels(m_config.channels, asrc->m_config.channels);
  if (m_config.profile != asrc->m_config.profile) {
    error_message = fmt::format(FY("The AAC profiles are different: {0} and {1}"), m_config.profile, asrc->m_config.profile);
    return CAN_CONNECT_NO_PARAMETERS;
  }

  return CAN_CONNECT_YES;
}
