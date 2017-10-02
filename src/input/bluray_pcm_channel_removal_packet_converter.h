/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definitions for the Blu-ray PCM channel removal converter

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/truehd.h"
#include "input/packet_converter.h"

class bluray_pcm_channel_removal_packet_converter_c: public packet_converter_c {
protected:
  std::size_t m_bytes_per_channel, m_num_input_channels, m_num_output_channels;

public:
  bluray_pcm_channel_removal_packet_converter_c(std::size_t bytes_per_channel, std::size_t num_input_channels, std::size_t num_output_channels);
  virtual ~bluray_pcm_channel_removal_packet_converter_c() {};

  virtual bool convert(packet_cptr const &packet);
};
