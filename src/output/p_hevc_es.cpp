/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   HEVC ES video output module

*/

#include "common/common_pch.h"

#include "common/codec.h"
#include "common/hacks.h"
#include "common/hevc/es_parser.h"
#include "merge/connection_checks.h"
#include "output/p_hevc_es.h"

hevc_es_video_packetizer_c::hevc_es_video_packetizer_c(generic_reader_c *p_reader,
                                                       track_info_c &p_ti)
  : avc_hevc_es_video_packetizer_c{p_reader, p_ti, "hevc", std::unique_ptr<mtx::avc_hevc::es_parser_c>(new mtx::hevc::es_parser_c)}
  , m_parser{static_cast<mtx::hevc::es_parser_c &>(*m_parser_base)}
{
  set_codec_id(MKV_V_MPEGH_HEVC);

  m_parser.normalize_parameter_sets(!mtx::hacks::is_engaged(mtx::hacks::DONT_NORMALIZE_PARAMETER_SETS));
}

connection_result_e
hevc_es_video_packetizer_c::can_connect_to(generic_packetizer_c *src,
                                           [[maybe_unused]] std::string &error_message) {
  auto *vsrc = dynamic_cast<hevc_es_video_packetizer_c *>(src);
  if (!vsrc)
    return CAN_CONNECT_NO_FORMAT;

  return CAN_CONNECT_YES;
}
