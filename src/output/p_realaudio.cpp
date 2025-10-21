/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   RealAudio output module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <matroska/KaxTracks.h>

#include "common/codec.h"
#include "merge/connection_checks.h"
#include "output/p_realaudio.h"

ra_packetizer_c::ra_packetizer_c(generic_reader_c *p_reader,
                                 track_info_c &p_ti,
                                 int samples_per_sec,
                                 int channels,
                                 int bits_per_sample,
                                 uint32_t fourcc)
  : generic_packetizer_c{p_reader, p_ti, track_audio}
  , m_samples_per_sec(samples_per_sec)
  , m_channels(channels)
  , m_bits_per_sample(bits_per_sample)
  , m_fourcc(fourcc)
{
  set_timestamp_factory_application_mode(TFA_SHORT_QUEUEING);
}

ra_packetizer_c::~ra_packetizer_c() {
}

void
ra_packetizer_c::set_headers() {
  std::string codec_id = fmt::format("A_REAL/{0}{1}{2}{3}",
                                      char(m_fourcc >> 24), char((m_fourcc >> 16) & 0xff), char((m_fourcc >> 8) & 0xff), char(m_fourcc & 0xff));
  set_codec_id(balg::to_upper_copy(codec_id));
  set_audio_sampling_freq(m_samples_per_sec);
  set_audio_channels(m_channels);
  set_audio_bit_depth(m_bits_per_sample);
  set_codec_private(m_ti.m_private_data);

  generic_packetizer_c::set_headers();
  m_track_entry->EnableLacing(false);
}

void
ra_packetizer_c::process_impl(packet_cptr const &packet) {
  add_packet(packet);
}

connection_result_e
ra_packetizer_c::can_connect_to(generic_packetizer_c *src,
                                std::string &error_message) {
  ra_packetizer_c *psrc = dynamic_cast<ra_packetizer_c *>(src);
  if (!psrc)
    return CAN_CONNECT_NO_FORMAT;

  connect_check_a_samplerate(m_samples_per_sec, psrc->m_samples_per_sec);
  connect_check_a_channels(m_channels,          psrc->m_channels);
  connect_check_a_bitdepth(m_bits_per_sample,   psrc->m_bits_per_sample);

  return CAN_CONNECT_YES;
}
