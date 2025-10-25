/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   MPEG 4 part 10 ES video output module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/avc/es_parser.h"
#include "common/codec.h"
#include "merge/connection_checks.h"
#include "merge/generic_reader.h"
#include "output/p_avc_es.h"

avc_es_video_packetizer_c::
avc_es_video_packetizer_c(generic_reader_c *p_reader,
                          track_info_c &p_ti,
                          uint32_t width,
                          uint32_t height)
  : xyzvc_es_video_packetizer_c{p_reader, p_ti, "avc", std::unique_ptr<mtx::xyzvc::es_parser_c>(new mtx::avc::es_parser_c), width, height}
  , m_parser{static_cast<mtx::avc::es_parser_c &>(*m_parser_base)}
{
  set_codec_id(MKV_V_MPEG4_AVC);

  m_parser.set_fix_bitstream_frame_rate(static_cast<bool>(m_ti.m_fix_bitstream_frame_rate));
}

void
avc_es_video_packetizer_c::check_if_default_duration_available()
  const {
  if (   !m_reader->is_providing_timestamps()
      && !m_timestamp_factory
      && !m_parser.is_default_duration_forced()
      && (   !m_parser.has_timing_info()
          || (   !m_parser.get_timing_info().fixed_frame_rate
              && (m_parser.get_timing_info().default_duration() < 5000000)))) // 200 fields/s
    mxwarn_tid(m_ti.m_fname, m_ti.m_id, Y("This AVC/H.264 track's timing information indicates that it uses a variable frame rate. "
                                          "However, no default duration nor an external timestamp file has been provided for it, nor does the source container provide timestamps. "
                                          "The resulting timestamps may not be useful.\n"));
}

connection_result_e
avc_es_video_packetizer_c::can_connect_to(generic_packetizer_c *src,
                                          std::string &error_message) {
  auto *vsrc = dynamic_cast<avc_es_video_packetizer_c *>(src);
  if (!vsrc)
    return CAN_CONNECT_NO_FORMAT;

  connect_check_codec_private(src);
  connect_check_v_width( m_width,  vsrc->m_width);
  connect_check_v_height(m_height, vsrc->m_height);

  return CAN_CONNECT_YES;
}
