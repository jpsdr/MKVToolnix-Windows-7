/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   WebVTT subtitle packetizer

   Written by Moritz Bunkus <mo@bunkus.online>.
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

void
webvtt_packetizer_c::process_impl(packet_cptr const &packet) {
  for (auto &addition : packet->data_adds)
    addition.data = memory_c::clone(mtx::string::normalize_line_endings(addition.data->to_string()));
  textsubs_packetizer_c::process_impl(packet);
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
