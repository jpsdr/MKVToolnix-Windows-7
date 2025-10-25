/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Fix channel layout for Blu-ray PCM

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "input/bluray_pcm_channel_layout_packet_converter.h"
#include "merge/generic_packetizer.h"

bluray_pcm_channel_layout_packet_converter_c::bluray_pcm_channel_layout_packet_converter_c(std::size_t bytes_per_channel,
                                                                                           std::size_t num_input_channels,
                                                                                           std::size_t num_output_channels)
  : packet_converter_c{nullptr}
  , m_bytes_per_channel{bytes_per_channel}
  , m_num_input_channels{num_input_channels}
  , m_num_output_channels{num_output_channels}
  , m_remap_buf_size{}
  , m_remap{}
{
  if (m_num_output_channels == 6) {
    m_remap_buf_size = m_bytes_per_channel * 3;
    m_remap          = &bluray_pcm_channel_layout_packet_converter_c::remap_6ch;
  } else if (m_num_output_channels == 7) {
    m_remap_buf_size = m_bytes_per_channel * 3;
    m_remap          = &bluray_pcm_channel_layout_packet_converter_c::remap_7ch;
  } else if (m_num_output_channels == 8) {
    m_remap_buf_size = m_bytes_per_channel * 5;
    m_remap          = &bluray_pcm_channel_layout_packet_converter_c::remap_8ch;
  }

  if (m_remap_buf_size > 0)
    m_remap_buf = memory_c::alloc(m_remap_buf_size);
}

void
bluray_pcm_channel_layout_packet_converter_c::removal(packet_cptr const &packet) {
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
}

void
bluray_pcm_channel_layout_packet_converter_c::remap_6ch(packet_cptr const &packet) {
  auto start_ptr = packet->data->get_buffer();
  auto end_ptr   = start_ptr + packet->data->get_size();

  // post-remap order: FL FR FC LFE BL BR
  while (start_ptr != end_ptr) {
    start_ptr += m_bytes_per_channel * 3;

    memcpy(m_remap_buf->get_buffer(),       start_ptr,                                           m_remap_buf_size);
    memcpy(start_ptr,                       m_remap_buf->get_buffer() + m_bytes_per_channel * 2, m_bytes_per_channel);
    memcpy(start_ptr + m_bytes_per_channel, m_remap_buf->get_buffer(),                           m_bytes_per_channel * 2);

    start_ptr += m_remap_buf_size;
  }
}

void
bluray_pcm_channel_layout_packet_converter_c::remap_7ch(packet_cptr const &packet) {
  auto start_ptr = packet->data->get_buffer();
  auto end_ptr   = start_ptr + packet->data->get_size();

  // post-remap order: FL FR FC BL BR SL SR
  while (start_ptr != end_ptr) {
    start_ptr += m_bytes_per_channel * 3;

    memcpy(m_remap_buf->get_buffer(),           start_ptr,                                       m_remap_buf_size);
    memcpy(start_ptr,                           m_remap_buf->get_buffer() + m_bytes_per_channel, m_bytes_per_channel * 2);
    memcpy(start_ptr + m_bytes_per_channel * 2, m_remap_buf->get_buffer(),                       m_bytes_per_channel);

    start_ptr += m_remap_buf_size + m_bytes_per_channel;
  }
}

void
bluray_pcm_channel_layout_packet_converter_c::remap_8ch(packet_cptr const &packet) {
  auto start_ptr = packet->data->get_buffer();
  auto end_ptr   = start_ptr + packet->data->get_size();

  // post-remap order: FL FR FC LFE BL BR SL SR
  while (start_ptr != end_ptr) {
    start_ptr += m_bytes_per_channel * 3;

    memcpy(m_remap_buf->get_buffer(),           start_ptr,                                           m_remap_buf_size);
    memcpy(start_ptr,                           m_remap_buf->get_buffer() + m_bytes_per_channel * 4, m_bytes_per_channel);
    // BL and BR stay at the same place
    memcpy(start_ptr + m_bytes_per_channel * 3, m_remap_buf->get_buffer(),                           m_bytes_per_channel);
    memcpy(start_ptr + m_bytes_per_channel * 4, m_remap_buf->get_buffer() + m_bytes_per_channel * 3, m_bytes_per_channel);

    start_ptr += m_remap_buf_size;
  }
}

bool
bluray_pcm_channel_layout_packet_converter_c::convert(packet_cptr const &packet) {
  // remove superfluous extra channel
  if ((m_num_output_channels % 2) != 0)
    removal(packet);

  // remap channels into WAVEFORMATEXTENSIBLE channel order
  if (m_remap_buf) {
    auto remainder = packet->data->get_size() % (m_bytes_per_channel * m_num_output_channels);
    if (remainder != 0)
      packet->data->set_size(packet->data->get_size() - remainder);

    (this->*m_remap)(packet);
  }

  m_ptzr->process(packet);

  return true;
}
