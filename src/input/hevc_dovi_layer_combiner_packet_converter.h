/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definitions for the HEVC DOVI layer combiner converter

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/debugging.h"
#include "input/packet_converter.h"
#include "output/p_hevc_es.h"

class hevc_dovi_layer_combiner_packet_converter_c: public packet_converter_c {
protected:
  hevc_es_video_packetizer_c *m_hevc_packetizer;
  uint16_t m_bl_pid, m_el_pid;
  debugging_option_c m_debug;

public:
  hevc_dovi_layer_combiner_packet_converter_c(hevc_es_video_packetizer_c *packetizer, uint16_t bl_pid, uint16_t el_pid);
  virtual ~hevc_dovi_layer_combiner_packet_converter_c() {};

  virtual bool convert(packet_cptr const &packet) override;
};
