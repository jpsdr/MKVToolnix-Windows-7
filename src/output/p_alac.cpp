/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   ALAC output module

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/alac.h"
#include "common/codec.h"
#include "common/hacks.h"
#include "merge/connection_checks.h"
#include "output/p_alac.h"

alac_packetizer_c::alac_packetizer_c(generic_reader_c *p_reader,
                                     track_info_c &p_ti,
                                     memory_cptr const &magic_cookie,
                                     unsigned int sample_rate,
                                     unsigned int channels)
  : generic_packetizer_c{p_reader, p_ti, track_audio}
  , m_magic_cookie{magic_cookie->clone()}
  , m_sample_rate{sample_rate}
  , m_channels{channels}
{
  set_codec_id(MKV_A_ALAC);

  set_audio_sampling_freq(static_cast<double>(m_sample_rate));
  set_audio_channels(m_channels);

  set_codec_private(magic_cookie);
}

alac_packetizer_c::~alac_packetizer_c() {
}

void
alac_packetizer_c::process_impl(packet_cptr const &packet) {
  add_packet(packet);
}

connection_result_e
alac_packetizer_c::can_connect_to(generic_packetizer_c *src,
                                  std::string &error_message) {
  alac_packetizer_c *alac_src = dynamic_cast<alac_packetizer_c *>(src);
  if (!alac_src)
    return CAN_CONNECT_NO_FORMAT;

  connect_check_a_samplerate(m_sample_rate, alac_src->m_sample_rate);
  connect_check_a_channels(  m_channels,    alac_src->m_channels);
  connect_check_codec_private(alac_src);

  return CAN_CONNECT_YES;
}
