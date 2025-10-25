/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   the Timed Text (tx3g) to text/UTF-8 converter

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/mm_mem_io.h"
#include "input/timed_text_to_text_utf8_converter.h"
#include "merge/generic_packetizer.h"

timed_text_to_text_utf8_converter_c::timed_text_to_text_utf8_converter_c(generic_packetizer_c &packetizer)
  : packet_converter_c{&packetizer}
{
}

bool
timed_text_to_text_utf8_converter_c::convert(packet_cptr const &packet) {
  if (packet->data->get_size() < 2)
    return true;

  mm_mem_io_c r{*packet->data};

  uint64_t text_length = r.read_uint16_be();
  text_length          = std::min<uint64_t>(text_length, r.get_size() - r.getFilePointer());

  if (text_length == 0)
    return true;

  auto packet_out  = packet;
  packet_out->data = r.read(text_length);

  // mxinfo(fmt::format("convert! at {0} size {1} text_length {2} text {3}\n", mtx::string::format_timestamp(packet->timestamp), packet->data->get_size(), text_length, packet_out->data->to_string()));

  m_ptzr->process(packet_out);

  return true;
}
