/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   AAC framing type converter

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "merge/generic_packetizer.h"
#include "input/aac_framing_packet_converter.h"

aac_framing_packet_converter_c::aac_framing_packet_converter_c(generic_packetizer_c *packetizer,
                                                               mtx::aac::parser_c::multiplex_type_e multiplex_type)
  : packet_converter_c{packetizer}
{
  m_parser.set_multiplex_type(multiplex_type);
}

bool
aac_framing_packet_converter_c::convert(packet_cptr const &packet) {
  if (packet->has_timestamp()) {
    m_parser.add_timestamp(timestamp_c::ns(packet->timestamp));
  }

  m_parser.add_bytes(packet->data);

  while (m_parser.frames_available()) {
    auto frame      = m_parser.get_frame();
    auto packet_out = std::make_shared<packet_t>(frame.m_data, frame.m_timestamp.to_ns(-1));
    m_ptzr->process(packet_out);
  }

  return true;
}
