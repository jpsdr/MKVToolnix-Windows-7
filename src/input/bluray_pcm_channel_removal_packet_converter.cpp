/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Removal of superfluous extra channel on Blu-ray PCM with odd number of channels

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "input/bluray_pcm_channel_removal_packet_converter.h"
#include "merge/generic_packetizer.h"

bluray_pcm_channel_removal_packet_converter_c::bluray_pcm_channel_removal_packet_converter_c(std::size_t bytes_per_channel,
                                                                                             std::size_t num_input_channels,
                                                                                             std::size_t num_output_channels)
  : packet_converter_c{nullptr}
  , m_bytes_per_channel{bytes_per_channel}
  , m_num_input_channels{num_input_channels}
  , m_num_output_channels{num_output_channels}
{
}

bool
bluray_pcm_channel_removal_packet_converter_c::convert(packet_cptr const &packet) {
  auto start_ptr                  = packet->data->get_buffer();
  auto end_ptr                    = start_ptr + packet->data->get_size();
  auto input_ptr                  = start_ptr;
  auto output_ptr                 = start_ptr;
  auto input_bytes_per_iteration  = m_bytes_per_channel * m_num_input_channels;
  auto output_bytes_per_iteration = m_bytes_per_channel * m_num_output_channels;

  while ((input_ptr + input_bytes_per_iteration) <= end_ptr) {
    if (input_ptr != output_ptr)
      std::memmove(output_ptr, input_ptr, output_bytes_per_iteration);

    input_ptr  += input_bytes_per_iteration;
    output_ptr += output_bytes_per_iteration;
  }

  packet->data->set_size(output_ptr - start_ptr);

  m_ptzr->process(packet);

  return true;
}
