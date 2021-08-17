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
                               std::string const &p_debug_type,
                               std::unique_ptr<mtx::avc_hevc::es_parser_c> &&parser_base)
  : generic_packetizer_c{p_reader, p_ti}
  , m_parser_base{std::move(parser_base)}
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

  m_parser_base->set_keep_ar_info(false);

  if (m_parser_default_duration_to_force)
    m_parser_base->force_default_duration(*m_parser_default_duration_to_force);
}

void
avc_hevc_es_video_packetizer_c::set_headers() {
  generic_packetizer_c::set_headers();

  m_track_entry->EnableLacing(false);
}

void
avc_hevc_es_video_packetizer_c::set_container_default_field_duration(int64_t default_duration) {
  m_parser_base->set_container_default_duration(default_duration);
}

unsigned int
avc_hevc_es_video_packetizer_c::get_nalu_size_length()
  const {
  return m_parser_base->get_nalu_size_length();
}

void
avc_hevc_es_video_packetizer_c::add_extra_data(memory_cptr const &data) {
  m_parser_base->add_bytes(data->get_buffer(), data->get_size());
}

void
avc_hevc_es_video_packetizer_c::flush_impl() {
  m_parser_base->flush();
  flush_frames();
}

// void
// hevc_es_video_packetizer_c::flush_frames() {
//   while (m_parser_base->frame_available()) {
//     if (m_first_frame) {
//       handle_delayed_headers();
//       m_first_frame = false;
//     }

//     auto frame = m_parser_base->get_frame();
//     add_packet(std::make_shared<packet_t>(frame.m_data, frame.m_start,
//                                           frame.m_end > frame.m_start ? frame.m_end - frame.m_start : m_htrack_default_duration,
//                                            frame.is_key_frame()       ? -1                          : frame.m_start + frame.m_ref1,
//                                           !frame.is_b_frame()         ? -1                          : frame.m_start + frame.m_ref2));
//   }
// }
