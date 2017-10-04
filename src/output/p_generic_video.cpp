/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Video for WIndows output module

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <matroska/KaxTracks.h>

#include "avilib.h"
#include "common/codec.h"
#include "common/debugging.h"
#include "common/endian.h"
#include "common/hacks.h"
#include "common/math.h"
#include "common/mpeg4_p2.h"
#include "merge/connection_checks.h"
#include "output/p_video_for_windows.h"

using namespace libmatroska;

generic_video_packetizer_c::generic_video_packetizer_c(generic_reader_c *p_reader,
                                                       track_info_c &p_ti,
                                                       std::string const &codec_id,
                                                       double fps,
                                                       int width,
                                                       int height)
  : generic_packetizer_c{p_reader, p_ti}
  , m_fps{fps}
  , m_width{width}
  , m_height{height}
  , m_frames_output{}
  , m_ref_timestamp{-1}
  , m_duration_shift{}
{
  set_track_type(track_video);

  set_codec_id(codec_id);
  set_codec_private(m_ti.m_private_data);
}

void
generic_video_packetizer_c::set_headers() {
  if (0.0 < m_fps)
    set_track_default_duration(1000000000.0 / m_fps);

  set_video_pixel_width(m_width);
  set_video_pixel_height(m_height);

  generic_packetizer_c::set_headers();

  m_track_entry->EnableLacing(false);
}

// Semantics:
// bref == -1: I frame, no references
// bref == -2: P or B frame, use timestamp of last I / P frame as the bref
// bref > 0:   P or B frame, use this bref as the backward reference
//             (absolute reference, not relative!)
// fref == 0:  P frame, no forward reference
// fref > 0:   B frame with given forward reference (absolute reference,
//             not relative!)
int
generic_video_packetizer_c::process(packet_cptr packet) {
  if ((0.0 == m_fps) && (-1 == packet->timestamp))
    mxerror_tid(m_ti.m_fname, m_ti.m_id, boost::format(Y("The FPS is 0.0 but the reader did not provide a timestamp for a packet. %1%\n")) % BUGMSG);

  if (-1 == packet->timestamp)
    packet->timestamp = (int64_t)(1000000000.0 * m_frames_output / m_fps) + m_duration_shift;

  if (-1 == packet->duration) {
    if (0.0 == m_fps)
      packet->duration = 0;
    else
      packet->duration = (int64_t)(1000000000.0 / m_fps);

  } else if (0.0 != packet->duration)
    m_duration_shift += packet->duration - (int64_t)(1000000000.0 / m_fps);

  ++m_frames_output;

  if (VFT_IFRAME == packet->bref)
    // Add a key frame and save its timestamp so that we can reference it later.
    m_ref_timestamp = packet->timestamp;

  else {
    // P or B frame. Use our last timestamp if the bref is -2, or the provided
    // one otherwise. The forward ref is always taken from the reader.
    if (VFT_PFRAMEAUTOMATIC == packet->bref)
      packet->bref = m_ref_timestamp;
    if (VFT_NOBFRAME == packet->fref)
      m_ref_timestamp = packet->timestamp;
  }

  add_packet(packet);

  return FILE_STATUS_MOREDATA;
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
