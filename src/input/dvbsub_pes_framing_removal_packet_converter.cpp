/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Removal of the PES framing used in MPEG transport streams for DVB subtitles

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/endian.h"
#include "common/timestamp.h"
#include "input/dvbsub_pes_framing_removal_packet_converter.h"
#include "merge/generic_packetizer.h"

dvbsub_pes_framing_removal_packet_converter_c::dvbsub_pes_framing_removal_packet_converter_c(generic_packetizer_c *packetizer)
  : packet_converter_c{packetizer}
{
}

dvbsub_pes_framing_removal_packet_converter_c::~dvbsub_pes_framing_removal_packet_converter_c() {
}

void
dvbsub_pes_framing_removal_packet_converter_c::flush() {
  if (!m_packet)
    return;

  if (!m_packet->has_duration())
    m_packet->duration = timestamp_c::ms(120).to_ns();

  m_ptzr->process(m_packet);
  m_packet.reset();
}

bool
dvbsub_pes_framing_removal_packet_converter_c::convert(packet_cptr const &packet) {
  if (m_packet) {
    if (!m_packet->has_duration() && m_packet->has_timestamp() && packet->has_timestamp())
      m_packet->duration = packet->timestamp - m_packet->timestamp;

    flush();
  }

  auto size = packet->data->get_size();

  // PES data bits:
  //  8: data identifier
  //  8: subtitle stream ID
  //  n: subtitling segments
  //  8: end of PES data field marker (0xff)

  // Subtitling segment bits:
  //  8: sync byte (0x0f)
  //  8: segment type
  // 16: page ID
  // 16: segment length
  //  n: segment data

  if (3 > size)
    return true;

  auto buffer_end = packet->data->get_buffer() + size;
  auto data_start = packet->data->get_buffer() + 2;
  auto data_end   = data_start;

  while (   ((data_end + 6) <= buffer_end)
         && (data_end[0] == 0x0f)) {
    auto segment_length = get_uint16_be(data_end + 4) + 6;
    if ((data_end + segment_length) <= buffer_end)
      data_end += segment_length;
    else
      break;
  }

  if (data_end <= data_start)
    return true;

  packet->data->set_offset(2);
  packet->data->set_size(data_end - data_start + 2);

  m_packet = packet;

  return true;
}
