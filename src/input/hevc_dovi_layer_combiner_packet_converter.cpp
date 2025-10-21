/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   HEVC DOVI layer combiner converter

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "input/hevc_dovi_layer_combiner_packet_converter.h"
#include "input/packet_converter.h"

hevc_dovi_layer_combiner_packet_converter_c::hevc_dovi_layer_combiner_packet_converter_c(hevc_es_video_packetizer_c *packetizer,
                                                                                         uint16_t bl_pid,
                                                                                         uint16_t el_pid)
  : packet_converter_c{packetizer}
  , m_hevc_packetizer{packetizer}
  , m_bl_pid{bl_pid}
  , m_el_pid{el_pid}
  , m_debug{"hevc_dovi_layer_combiner|mpeg_ts_dovi|mpeg_ts|dovi"}
{
  mxdebug_if(m_debug, fmt::format("new instance with base layer PID {0} enhancement layer PID {1}\n", bl_pid, el_pid));

  m_hevc_packetizer->enable_dovi_layer_combiner();
}

bool
hevc_dovi_layer_combiner_packet_converter_c::convert(packet_cptr const &packet) {
  auto is_base_layer = m_current_pid == m_bl_pid;
  mxdebug_if(m_debug, fmt::format("current PID {0} ({1} layer)\n", m_current_pid, is_base_layer ? "base" : "enhancement"));

  if (is_base_layer)
    m_hevc_packetizer->process(packet);
  else
    m_hevc_packetizer->process_enhancement_layer(packet);

  return true;
}
