/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   WebVTT subtitle packetizer

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/codec.h"
#include "common/strings/editing.h"
#include "merge/connection_checks.h"
#include "output/p_webvtt.h"

webvtt_packetizer_c::webvtt_packetizer_c(generic_reader_c *p_reader,
                                         track_info_c &p_ti)
  : textsubs_packetizer_c{p_reader, p_ti, MKV_S_TEXTWEBVTT}
{
  m_line_ending_style = mtx::string::line_ending_style_e::lf;
}

webvtt_packetizer_c::~webvtt_packetizer_c() {
}

int
webvtt_packetizer_c::process(packet_cptr packet) {
  for (auto &addition : packet->data_adds)
    addition = memory_c::clone(mtx::string::normalize_line_endings(addition->to_string()));

  return textsubs_packetizer_c::process(packet);
}

connection_result_e
webvtt_packetizer_c::can_connect_to(generic_packetizer_c *src,
                                    std::string &error_message) {
  auto psrc = dynamic_cast<webvtt_packetizer_c *>(src);
  if (!psrc)
    return CAN_CONNECT_NO_FORMAT;

  connect_check_codec_private(src);

  return CAN_CONNECT_YES;
}
