/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Splitting TrueHD/AC-3 into to packetizers

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "input/truehd_ac3_splitting_packet_converter.h"
#include "merge/generic_packetizer.h"
#include "output/p_truehd.h"

truehd_ac3_splitting_packet_converter_c::truehd_ac3_splitting_packet_converter_c(generic_packetizer_c *truehd_ptzr,
                                                                                 generic_packetizer_c *ac3_ptzr)
  : packet_converter_c{truehd_ptzr}
  , m_ac3_ptzr{ac3_ptzr}
  , m_truehd_timestamp{-1}
  , m_ac3_timestamp{-1}
{
}

bool
truehd_ac3_splitting_packet_converter_c::convert(packet_cptr const &packet) {
  m_parser.add_data(packet->data->get_buffer(), packet->data->get_size());
  m_parser.parse(true);

  m_truehd_timestamp = packet->timestamp;
  m_ac3_timestamp    = packet->timestamp;

  process_frames();

  return true;
}

void
truehd_ac3_splitting_packet_converter_c::set_ac3_packetizer(generic_packetizer_c *ac3_ptzr) {
  m_ac3_ptzr = ac3_ptzr;
}

void
truehd_ac3_splitting_packet_converter_c::flush() {
  m_parser.parse(true);

  process_frames();

  if (m_ptzr)
    m_ptzr->flush();

  if (m_ac3_ptzr)
    m_ac3_ptzr->flush();
}

void
truehd_ac3_splitting_packet_converter_c::process_frames() {
  while (m_parser.frame_available()) {
    auto frame = m_parser.get_next_frame();

    if ((frame->is_truehd() || frame->is_mlp()) && m_ptzr) {
      static_cast<truehd_packetizer_c *>(m_ptzr)->process_framed(frame, m_truehd_timestamp);
      m_truehd_timestamp = -1;

    } else if (frame->is_ac3() && m_ac3_ptzr) {
      m_ac3_ptzr->process(std::make_shared<packet_t>(frame->m_data, m_ac3_timestamp));
      m_ac3_timestamp = -1;
    }
  }
}
