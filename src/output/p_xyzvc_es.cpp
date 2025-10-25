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

#include <matroska/KaxTracks.h>

#include "common/xyzvc/types.h"
#include "merge/generic_reader.h"
#include "merge/output_control.h"
#include "output/p_xyzvc_es.h"

xyzvc_es_video_packetizer_c::
xyzvc_es_video_packetizer_c(generic_reader_c *p_reader,
                            track_info_c &p_ti,
                            std::string const &p_debug_type,
                            std::unique_ptr<mtx::xyzvc::es_parser_c> &&parser_base,
                            uint32_t width,
                            uint32_t height)
  : generic_packetizer_c{p_reader, p_ti, track_video}
  , m_parser_base{std::move(parser_base)}
  , m_width{width}
  , m_height{height}
  , m_debug_timestamps{  fmt::format("{0}_es|{0}_es_timestamps",   p_debug_type)}
  , m_debug_aspect_ratio{fmt::format("{0}_es|{0}_es_aspect_ratio", p_debug_type)}
{
  m_relaxed_timestamp_checking = true;

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

  set_video_pixel_dimensions(m_width, m_height);
}

void
xyzvc_es_video_packetizer_c::set_headers() {
  generic_packetizer_c::set_headers();

  m_track_entry->EnableLacing(false);
}

void
xyzvc_es_video_packetizer_c::set_container_default_field_duration(int64_t default_duration) {
  m_parser_base->set_container_default_duration(default_duration);
}

void
xyzvc_es_video_packetizer_c::set_source_timestamp_resolution(int64_t resolution) {
  m_source_timestamp_resolution = resolution;
}

void
xyzvc_es_video_packetizer_c::set_is_framed(unsigned int nalu_size) {
  m_framed_nalu_size = nalu_size;
}

unsigned int
xyzvc_es_video_packetizer_c::get_nalu_size_length()
  const {
  return m_parser_base->get_nalu_size_length();
}

void
xyzvc_es_video_packetizer_c::add_extra_data(memory_cptr const &data) {
  m_parser_base->add_bytes(data->get_buffer(), data->get_size());
}

void
xyzvc_es_video_packetizer_c::process_impl(packet_cptr const &packet) {
  try {
    if (packet->has_timestamp())
      m_parser_base->add_timestamp(packet->timestamp);

    if (m_framed_nalu_size)
      m_parser_base->add_bytes_framed(packet->data->get_buffer(), packet->data->get_size(), *m_framed_nalu_size);
    else
      m_parser_base->add_bytes(packet->data->get_buffer(), packet->data->get_size());
    flush_frames();

  } catch (mtx::exception &error) {
    mxerror_tid(m_ti.m_fname, m_ti.m_id,
                fmt::format("{0} {1}\n{2}\n",
                            Y("mkvmerge encountered broken or unparsable data in this video track."),
                            Y("The error message was:"),
                            error.error()));
  }
}

void
xyzvc_es_video_packetizer_c::flush_impl() {
  m_parser_base->flush();
  flush_frames();
}

int64_t
xyzvc_es_video_packetizer_c::calculate_frame_duration(mtx::xyzvc::frame_t const &frame)
  const {
  auto duration = frame.m_end > frame.m_start ? frame.m_end - frame.m_start : m_htrack_default_duration;

  if (   (m_htrack_default_duration > 0)
      && m_source_timestamp_resolution
      && (std::abs(duration - m_htrack_default_duration) <= *m_source_timestamp_resolution))
    duration = m_htrack_default_duration;

  return duration;
}

void
xyzvc_es_video_packetizer_c::flush_frames() {
  while (m_parser_base->frame_available()) {
    if (m_first_frame) {
      handle_delayed_headers();
      m_first_frame = false;
    }

    auto frame    = m_parser_base->get_frame();
    auto duration = calculate_frame_duration(frame);
    auto packet   = std::make_shared<packet_t>(frame.m_data, frame.m_start, duration,
                                                frame.is_key_frame() ? -1 : frame.m_start + frame.m_ref1,
                                               !frame.is_b_frame()   ? -1 : frame.m_start + frame.m_ref2);

    packet->key_flag         = frame.is_key_frame();
    packet->discardable_flag = frame.is_discardable();

    add_packet(packet);
  }
}

void
xyzvc_es_video_packetizer_c::check_if_default_duration_available()
  const {
  // No default implementation, but not required either.
}

void
xyzvc_es_video_packetizer_c::handle_delayed_headers() {
  if (0 < m_parser_base->get_num_skipped_frames())
    mxwarn_tid(m_ti.m_fname, m_ti.m_id, fmt::format(FY("This AVC/H.264 track does not start with a key frame. The first {0} frames have been skipped.\n"), m_parser_base->get_num_skipped_frames()));

  set_codec_private(m_parser_base->get_configuration_record());

  check_if_default_duration_available();

  handle_aspect_ratio();
  handle_actual_default_duration();

  rerender_track_headers();
}

void
xyzvc_es_video_packetizer_c::handle_aspect_ratio() {
  mxdebug_if(m_debug_aspect_ratio, fmt::format("already set? {0} has par been found? {1}\n", display_dimensions_or_aspect_ratio_set(), m_parser_base->has_par_been_found()));

  if (display_dimensions_or_aspect_ratio_set() || !m_parser_base->has_par_been_found())
    return;

  auto dimensions = m_parser_base->get_display_dimensions(m_hvideo_pixel_width, m_hvideo_pixel_height);
  set_video_display_dimensions(dimensions.first, dimensions.second, generic_packetizer_c::ddu_pixels, option_source_e::bitstream);

  mxinfo_tid(m_ti.m_fname, m_ti.m_id,
             fmt::format(FY("Extracted the aspect ratio information from the video bitstream and set the display dimensions to {0}/{1}.\n"),
                         m_ti.m_display_width, m_ti.m_display_height));

  mxdebug_if(m_debug_aspect_ratio,
             fmt::format("PAR {0} pixel_width/hgith {1}/{2} display_width/height {3}/{4}\n",
                         m_parser_base->get_par(), m_hvideo_pixel_width, m_hvideo_pixel_height, m_ti.m_display_width, m_ti.m_display_height));
}

void
xyzvc_es_video_packetizer_c::handle_actual_default_duration() {
  int64_t actual_default_duration = m_parser_base->get_most_often_used_duration();
  mxdebug_if(m_debug_timestamps, fmt::format("Most often used duration: {0} forced? {1} current default duration: {2}\n", actual_default_duration, m_default_duration_forced, m_htrack_default_duration));

  if (   !m_default_duration_forced
      && (0 < actual_default_duration)
      && (m_htrack_default_duration != actual_default_duration))
    set_track_default_duration(actual_default_duration);

  else if (   m_default_duration_forced
           && (0 < m_default_duration_for_interlaced_content)
           && (std::abs(actual_default_duration - m_default_duration_for_interlaced_content) <= 20000)) {
    m_default_duration_forced = false;
    set_track_default_duration(m_default_duration_for_interlaced_content);
  }
}

void
xyzvc_es_video_packetizer_c::connect(generic_packetizer_c *src,
                                        int64_t p_append_timestamp_offset) {
  generic_packetizer_c::connect(src, p_append_timestamp_offset);

  if (2 != m_connected_to)
    return;

  auto real_src = dynamic_cast<xyzvc_es_video_packetizer_c *>(src);
  assert(real_src);

  m_htrack_default_duration = real_src->m_htrack_default_duration;
  m_default_duration_forced = real_src->m_default_duration_forced;

  if (m_default_duration_forced && (-1 != m_htrack_default_duration)) {
    m_default_duration_for_interlaced_content = m_htrack_default_duration / 2;
    m_parser_base->force_default_duration(m_default_duration_for_interlaced_content);
  }
}
