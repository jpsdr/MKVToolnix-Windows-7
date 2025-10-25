/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Video for WIndows output module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <matroska/KaxTracks.h>

#include "avilib.h"
#include "common/codec.h"
#include "common/common_urls.h"
#include "common/debugging.h"
#include "common/endian.h"
#include "common/hacks.h"
#include "common/mpeg4_p2.h"
#include "merge/connection_checks.h"
#include "output/p_video_for_windows.h"

generic_video_packetizer_c::generic_video_packetizer_c(generic_reader_c *p_reader,
                                                       track_info_c &p_ti,
                                                       std::string const &codec_id,
                                                       int64_t default_duration,
                                                       int width,
                                                       int height)
  : generic_packetizer_c{p_reader, p_ti, track_video}
  , m_width{width}
  , m_height{height}
  , m_frames_output{}
  , m_default_duration{default_duration}
  , m_ref_timestamp{-1}
  , m_duration_shift{}
  , m_bframe_bref{-1}
{
  set_codec_id(codec_id);
  set_codec_private(m_ti.m_private_data);
}

void
generic_video_packetizer_c::set_headers() {
  if (0 < m_default_duration)
    set_track_default_duration(m_default_duration);

  set_video_pixel_dimensions(m_width, m_height);

  generic_packetizer_c::set_headers();

  m_track_entry->EnableLacing(false);
}

// Semantics:
// bref == -1: I frame, no references
// bref == -2: P or B frame, use timestamp of last I / P frame as the bref
// bref >= 0:  P or B frame, use this bref as the backward reference
//             (absolute reference, not relative!)
// fref == -1: I or P frame, no forward reference
// fref > 0:   B frame with given forward reference (absolute reference,
//             not relative!)
void
generic_video_packetizer_c::process_impl(packet_cptr const &packet) {
  if ((0 == m_default_duration) && (-1 == packet->timestamp))
    mxerror_tid(m_ti.m_fname, m_ti.m_id, fmt::format(FY("The FPS is 0.0 but the reader did not provide a timestamp for a packet. {0}\n"), BUGMSG));

  if (-1 == packet->timestamp)
    packet->timestamp = m_frames_output * m_default_duration + m_duration_shift;

  if (-1 == packet->duration)
    packet->duration = m_default_duration;

  else if (0 != packet->duration)
    m_duration_shift += packet->duration - m_default_duration;

  ++m_frames_output;

  // Reference the last I or P frame
  if (VFT_PFRAMEAUTOMATIC == packet->bref)
    packet->bref = m_ref_timestamp;

  if (VFT_NOBFRAME == packet->fref) {
    // Save timestamp of the last I or P frame for "broken" B frames
    m_bframe_bref = m_ref_timestamp;
    // Save timestamp of I or P frame so that we can reference it later
    m_ref_timestamp = packet->timestamp;
  }
  // Handle "broken" B frames
  else if (VFT_IFRAME == packet->bref) { // expect VFT_NOBFRAME < packet->fref
    packet->bref = m_bframe_bref;
  }

  add_packet(packet);
}

connection_result_e
generic_video_packetizer_c::can_connect_to(generic_packetizer_c *src,
                                           std::string &error_message) {
  auto vsrc = dynamic_cast<generic_video_packetizer_c *>(src);
  if (!vsrc)
    return CAN_CONNECT_NO_FORMAT;

  connect_check_v_width(m_width,      vsrc->m_width);
  connect_check_v_height(m_height,    vsrc->m_height);
  connect_check_codec_id(m_hcodec_id, vsrc->m_hcodec_id);
  connect_check_codec_private(vsrc);

  return CAN_CONNECT_YES;
}
