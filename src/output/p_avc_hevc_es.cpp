/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   MPEG 4 part 10 ES video output module

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <matroska/KaxTracks.h>

#include "merge/generic_reader.h"
#include "merge/output_control.h"
#include "output/p_avc_hevc_es.h"

avc_hevc_es_video_packetizer_c::
avc_hevc_es_video_packetizer_c(generic_reader_c *p_reader,
                               track_info_c &p_ti,
                               std::string const &p_debug_type)
  : generic_packetizer_c{p_reader, p_ti}
  , m_debug_timestamps{  fmt::format("{0}_es|{0}_es_timestamps",   p_debug_type)}
  , m_debug_aspect_ratio{fmt::format("{0}_es|{0}_es_aspect_ratio", p_debug_type)}
{
  m_relaxed_timestamp_checking = true;

  set_track_type(track_video);

  // If no external timestamp file has been specified then mkvmerge
  // might have created a factory due to the --default-duration
  // command line argument. This factory must be disabled for the AVC
  // packetizer because it takes care of handling the default
  // duration/FPS itself.
  if (m_ti.m_ext_timestamps.empty())
    m_timestamp_factory.reset();

  int64_t factory_default_duration;
  if (m_timestamp_factory && (-1 != (factory_default_duration = m_timestamp_factory->get_default_duration(-1)))) {
    set_track_default_duration(factory_default_duration);
    m_parser_default_duration_to_force = factory_default_duration;
    m_default_duration_forced          = true;
    mxdebug_if(m_debug_timestamps, fmt::format("Forcing default duration due to timestamp factory to {0}\n", m_htrack_default_duration));

  } else if (m_default_duration_forced && (-1 != m_htrack_default_duration)) {
    m_default_duration_for_interlaced_content = m_htrack_default_duration / 2;
    m_parser_default_duration_to_force        = m_default_duration_for_interlaced_content;
    mxdebug_if(m_debug_timestamps, fmt::format("Forcing default duration due to --default-duration to {0}\n", m_htrack_default_duration));
  }
}

void
avc_hevc_es_video_packetizer_c::set_headers() {
  generic_packetizer_c::set_headers();

  m_track_entry->EnableLacing(false);
}
