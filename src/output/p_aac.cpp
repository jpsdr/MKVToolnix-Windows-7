/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   AAC output module

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/aac.h"
#include "common/codec.h"
#include "merge/connection_checks.h"
#include "output/p_aac.h"

using namespace libmatroska;

aac_packetizer_c::aac_packetizer_c(generic_reader_c *p_reader,
                                   track_info_c &p_ti,
                                   int profile,
                                   int samples_per_sec,
                                   int channels,
                                   mode_e mode)
  : generic_packetizer_c(p_reader, p_ti)
  , m_samples_per_sec(samples_per_sec)
  , m_channels(channels)
  , m_profile(profile)
  , m_mode{mode}
  , m_timestamp_calculator{static_cast<int64_t>(m_samples_per_sec)}
  , m_packet_duration{m_timestamp_calculator.get_duration(ms_samples_per_packet).to_ns()}
{
  set_track_type(track_audio);
  set_track_default_duration(m_packet_duration);
}

aac_packetizer_c::~aac_packetizer_c() {
}

void
aac_packetizer_c::set_headers() {
  set_codec_id(MKV_A_AAC);
  set_audio_sampling_freq((float)m_samples_per_sec);
  set_audio_channels(m_channels);

  if (m_ti.m_private_data && (0 < m_ti.m_private_data->get_size()))
    set_codec_private(m_ti.m_private_data);

  else {
    aac::audio_config_t audio_config{};
    audio_config.profile            = AAC_PROFILE_SBR == m_profile ? AAC_PROFILE_LC : m_profile;
    audio_config.channels           = m_channels;
    audio_config.sample_rate        = m_samples_per_sec;
    audio_config.output_sample_rate = AAC_PROFILE_SBR == m_profile ? m_samples_per_sec * 2 : m_samples_per_sec;
    audio_config.sbr                = AAC_PROFILE_SBR == m_profile;

    set_codec_private(aac::create_audio_specific_config(audio_config));
  }

  generic_packetizer_c::set_headers();
}

int
aac_packetizer_c::process_headerless(packet_cptr packet) {
  packet->timecode = m_timestamp_calculator.get_next_timestamp(ms_samples_per_packet).to_ns();
  packet->duration = m_packet_duration;

  add_packet(packet);

  return FILE_STATUS_MOREDATA;
}

int
aac_packetizer_c::process(packet_cptr packet) {
  m_timestamp_calculator.add_timestamp(packet);

  if (m_mode == mode_e::headerless)
    return process_headerless(packet);

  m_parser.add_bytes(packet->data);

  if (!m_parser.headers_parsed())
    return FILE_STATUS_MOREDATA;

  while (m_parser.frames_available()) {
    auto frame = m_parser.get_frame();

    process_headerless(std::make_shared<packet_t>(frame.m_data));

    if (verbose && frame.m_garbage_size)
      mxwarn_tid(m_ti.m_fname, m_ti.m_id, boost::format(Y("Skipping %1% bytes (no valid AAC header found). This might cause audio/video desynchronisation.\n")) % frame.m_garbage_size);
  }

  return FILE_STATUS_MOREDATA;
}

connection_result_e
aac_packetizer_c::can_connect_to(generic_packetizer_c *src,
                                 std::string &error_message) {
  aac_packetizer_c *asrc = dynamic_cast<aac_packetizer_c *>(src);
  if (!asrc)
    return CAN_CONNECT_NO_FORMAT;

  connect_check_a_samplerate(m_samples_per_sec, asrc->m_samples_per_sec);
  connect_check_a_channels(m_channels, asrc->m_channels);
  if (m_profile != asrc->m_profile) {
    error_message = (boost::format(Y("The AAC profiles are different: %1% and %2%")) % m_profile % asrc->m_profile).str();
    return CAN_CONNECT_NO_PARAMETERS;
  }

  return CAN_CONNECT_YES;
}
