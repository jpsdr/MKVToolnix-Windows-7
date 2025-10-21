/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   VPX video output module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/codec.h"
#include "common/ivf.h"
#include "common/vp9.h"
#include "merge/connection_checks.h"
#include "merge/output_control.h"
#include "output/p_vpx.h"

vpx_video_packetizer_c::vpx_video_packetizer_c(generic_reader_c *p_reader,
                                               track_info_c &p_ti,
                                               codec_c::type_e p_codec)
  : generic_packetizer_c{p_reader, p_ti, track_video}
  , m_previous_timestamp(-1)
  , m_codec{p_codec}
  , m_is_vp9{p_codec == codec_c::type_e::V_VP9}
{
  m_timestamp_factory_application_mode = TFA_SHORT_QUEUEING;

  set_codec_id(m_is_vp9 ? MKV_V_VP9 : MKV_V_VP8);
  set_codec_private(p_ti.m_private_data);
}

void
vpx_video_packetizer_c::set_headers() {
  generic_packetizer_c::set_headers();
}

void
vpx_video_packetizer_c::vp9_determine_codec_private(memory_c const &mem) {
  // See https://www.webmproject.org/docs/container/#vp9-codec-feature-metadata-codecprivate

  if (!m_is_vp9 || m_hcodec_private)
    return;

  auto header_data = mtx::vp9::parse_header_data(mem);
  if (!header_data)
    return;

  auto size = 1 + 1 + 1         // profile
            + 1 + 1 + 1;        // bit depth

  if (! (!header_data->subsampling_x && header_data->subsampling_y)) // 4:4:0 not supported.
    size += 1 + 1 + 1;

  if (header_data->level)
    size += 1 + 1 + 1;

  auto codec_private = memory_c::alloc(size);
  auto ptr           = codec_private->get_buffer();

  // profile
  ptr[0] = 1;                   // ID
  ptr[1] = 1;                   // length
  ptr[2] = header_data->profile;
  ptr   += 3;

  if (header_data->level) {
    // level
    ptr[0] = 2;                 // ID
    ptr[1] = 1;                 // length
    ptr[2] = *header_data->level;
    ptr   += 3;
  }

  // bit depth
  ptr[0] = 3;                   // ID
  ptr[1] = 1;                   // length
  ptr[2] = header_data->bit_depth;
  ptr   += 3;

  if (! (!header_data->subsampling_x && header_data->subsampling_y)) { // 4:4:0 not supported.
    // chroma subsampling
    ptr[0] = 4;                   // ID
    ptr[1] = 1;                   // length
    ptr[2] = (0 == header_data->profile) || (2 == header_data->profile) ? 1  // 4:2:0 colocated with luma
           : !header_data->subsampling_x && !header_data->subsampling_y ? 3  // 4:4:4
           :  header_data->subsampling_x && !header_data->subsampling_y ? 2  // 4:2:2
           :                                                              1; // 4:2:0 colocated with luma
    ptr   += 3;
  }

  set_codec_private(codec_private);
  rerender_track_headers();
}

void
vpx_video_packetizer_c::process_impl(packet_cptr const &packet) {
  vp9_determine_codec_private(*packet->data);

  packet->bref         = ivf::is_keyframe(packet->data, m_codec) ? -1 : m_previous_timestamp;
  m_previous_timestamp = packet->timestamp;

  add_packet(packet);
}

connection_result_e
vpx_video_packetizer_c::can_connect_to(generic_packetizer_c *src,
                                       std::string &error_message) {
  vpx_video_packetizer_c *psrc = dynamic_cast<vpx_video_packetizer_c *>(src);
  if (!psrc)
    return CAN_CONNECT_NO_FORMAT;

  connect_check_codec_id(m_hcodec_id, psrc->m_hcodec_id);

  return CAN_CONNECT_YES;
}

bool
vpx_video_packetizer_c::is_compatible_with(output_compatibility_e compatibility) {
  return (OC_MATROSKA == compatibility) || (OC_WEBM == compatibility);
}
