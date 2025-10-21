/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definitions for the Blu-ray PCM channel layout converter

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/truehd.h"
#include "input/packet_converter.h"

class bluray_pcm_channel_layout_packet_converter_c: public packet_converter_c {
protected:
  std::size_t m_bytes_per_channel, m_num_input_channels, m_num_output_channels, m_remap_buf_size;
  memory_cptr m_remap_buf;
  void (bluray_pcm_channel_layout_packet_converter_c::*m_remap)(packet_cptr const &packet);

public:
  bluray_pcm_channel_layout_packet_converter_c(std::size_t bytes_per_channel, std::size_t num_input_channels, std::size_t num_output_channels);
  virtual ~bluray_pcm_channel_layout_packet_converter_c() {};

  virtual void removal(packet_cptr const &packet);
  virtual void remap_6ch(packet_cptr const &packet);
  virtual void remap_7ch(packet_cptr const &packet);
  virtual void remap_8ch(packet_cptr const &packet);
  virtual bool convert(packet_cptr const &packet);
};
