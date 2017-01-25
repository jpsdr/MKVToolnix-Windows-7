/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   DVB subtitles packetizer

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <matroska/KaxTracks.h>

#include "common/codec.h"
#include "output/p_dvbsub.h"

using namespace libmatroska;

dvbsub_packetizer_c::dvbsub_packetizer_c(generic_reader_c *reader,
                                         track_info_c &ti,
                                         memory_cptr const &private_data)
  : generic_packetizer_c(reader, ti)
{
  set_track_type(track_subtitle);
  set_default_compression_method(COMPRESSION_ZLIB);
  m_ti.m_private_data = private_data->clone();
}

dvbsub_packetizer_c::~dvbsub_packetizer_c() {
}

void
dvbsub_packetizer_c::set_headers() {
  set_codec_id(MKV_S_DVBSUB);
  set_codec_private(m_ti.m_private_data);

  generic_packetizer_c::set_headers();

  m_track_entry->EnableLacing(false);
}

int
dvbsub_packetizer_c::process(packet_cptr packet) {
  packet->duration_mandatory = true;
  add_packet(packet);

  return FILE_STATUS_MOREDATA;
}

connection_result_e
dvbsub_packetizer_c::can_connect_to(generic_packetizer_c *src,
                                    std::string &) {
  auto vsrc = dynamic_cast<dvbsub_packetizer_c *>(src);
  if (!vsrc)
    return CAN_CONNECT_NO_FORMAT;
  return CAN_CONNECT_YES;
}
