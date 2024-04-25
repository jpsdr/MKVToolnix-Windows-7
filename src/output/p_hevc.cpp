/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   HEVC video output module

*/

#include "common/common_pch.h"

#include <cstdlib>

#include "common/codec.h"
#include "common/endian.h"
#include "common/hacks.h"
#include "common/hevc/util.h"
#include "common/hevc/es_parser.h"
#include "common/strings/formatting.h"
#include "merge/connection_checks.h"
#include "merge/output_control.h"
#include "output/p_hevc.h"

class hevc_video_packetizer_private_c {
public:
  int nalu_size_len{};
  int64_t max_nalu_size{}, source_timestamp_resolution{1};
  std::shared_ptr<mtx::hevc::es_parser_c> parser{new mtx::hevc::es_parser_c};
  bool flushed{};
};

hevc_video_packetizer_c::
hevc_video_packetizer_c(generic_reader_c *p_reader,
                        track_info_c &p_ti,
                        int64_t default_duration,
                        int width,
                        int height)
  : generic_video_packetizer_c{p_reader, p_ti, MKV_V_MPEGH_HEVC, default_duration, width, height}
  , p_ptr{new hevc_video_packetizer_private_c}
{
  auto &p = *p_func();

  m_relaxed_timestamp_checking = true;

  if (23 <= m_ti.m_private_data->get_size())
    p_ptr->nalu_size_len = (m_ti.m_private_data->get_buffer()[21] & 0x03) + 1;

  set_codec_private(m_ti.m_private_data);

  p.parser->set_normalize_parameter_sets(!mtx::hacks::is_engaged(mtx::hacks::DONT_NORMALIZE_PARAMETER_SETS));
  p.parser->set_configuration_record(m_hcodec_private);
}

void
hevc_video_packetizer_c::set_source_timestamp_resolution(int64_t resolution) {
  p_func()->source_timestamp_resolution = resolution;
}

void
hevc_video_packetizer_c::setup_default_duration() {
  auto &p                      = *p_func();

  auto source_default_duration = m_htrack_default_duration > 0 ? m_htrack_default_duration
                               : m_default_duration        > 0 ? m_default_duration
                               :                                 0;
  auto stream_default_duration = p.parser->has_stream_default_duration() ? p.parser->get_stream_default_duration() : 0;
  auto diff_source_stream      = std::abs(stream_default_duration - source_default_duration);
  auto output_default_duration = stream_default_duration && !source_default_duration                             ? stream_default_duration
                               : stream_default_duration && (diff_source_stream < p.source_timestamp_resolution) ? stream_default_duration
                               :                                                                                   source_default_duration;

  if (source_default_duration > 0)
    p.parser->set_container_default_duration(source_default_duration);

  if (output_default_duration > 0)
    set_track_default_duration(output_default_duration);
}

void
hevc_video_packetizer_c::set_headers() {
  setup_default_duration();

  if (m_ti.m_private_data && m_ti.m_private_data->get_size())
    extract_aspect_ratio();

  if (m_ti.m_private_data && m_ti.m_private_data->get_size() && m_ti.m_fix_bitstream_frame_rate)
    set_codec_private(m_ti.m_private_data);

  generic_video_packetizer_c::set_headers();
}

void
hevc_video_packetizer_c::extract_aspect_ratio() {
  auto result = mtx::hevc::extract_par(m_ti.m_private_data);

  set_codec_private(result.new_hevcc);

  if (!result.is_valid() || display_dimensions_or_aspect_ratio_set())
    return;

  auto par = mtx::rational(result.numerator, result.denominator);

  set_video_display_dimensions(1 <= par ? mtx::to_int_rounded(m_width * par) : m_width,
                               1 <= par ? m_height                           : mtx::to_int_rounded(m_height / par),
                               generic_packetizer_c::ddu_pixels,
                               option_source_e::bitstream);

  mxinfo_tid(m_ti.m_fname, m_ti.m_id,
             fmt::format(FY("Extracted the aspect ratio information from the HEVC video data and set the display dimensions to {0}/{1}.\n"),
                         m_ti.m_display_width, m_ti.m_display_height));
}

void
hevc_video_packetizer_c::process_impl(packet_cptr const &packet) {
  auto &p = *p_func();

  if (packet->is_key_frame() && (VFT_PFRAMEAUTOMATIC != packet->bref))
    p.parser->set_next_i_slice_is_key_frame();

  if (packet->has_timestamp())
    p.parser->add_timestamp(packet->timestamp);

  p.parser->add_bytes_framed(packet->data, p.nalu_size_len);

  flush_frames();
}

connection_result_e
hevc_video_packetizer_c::can_connect_to(generic_packetizer_c *src,
                                        std::string &error_message) {
  auto vsrc = dynamic_cast<hevc_video_packetizer_c *>(src);
  if (!vsrc)
    return CAN_CONNECT_NO_FORMAT;

  connect_check_v_width( m_width,  vsrc->m_width);
  connect_check_v_height(m_height, vsrc->m_height);

  return CAN_CONNECT_YES;
}

void
hevc_video_packetizer_c::connect(generic_packetizer_c *src,
                                 int64_t append_timestamp_offset) {
  auto &p = *p_func();

  generic_packetizer_c::connect(src, append_timestamp_offset);

  if (1 == m_connected_to)
    p.parser = static_cast<hevc_video_packetizer_c *>(src)->p_func()->parser;
}

void
hevc_video_packetizer_c::flush_impl() {
  auto &p = *p_func();

  if (p.flushed)
    return;

  p.flushed = true;

  p.parser->flush();
  flush_frames();
}

void
hevc_video_packetizer_c::flush_frames() {
  auto &p = *p_func();

  while (p.parser->frame_available()) {
    auto frame                    = p.parser->get_frame();
    auto duration                 = frame.m_end > frame.m_start ? frame.m_end - frame.m_start : m_htrack_default_duration;
    auto diff_to_default_duration = std::abs(duration - m_htrack_default_duration);

    if (diff_to_default_duration < p.source_timestamp_resolution)
      duration = m_htrack_default_duration;

    add_packet(std::make_shared<packet_t>(frame.m_data, frame.m_start, duration,
                                           frame.is_key_frame() ? -1 : frame.m_start + frame.m_ref1,
                                          !frame.is_b_frame()   ? -1 : frame.m_start + frame.m_ref2));
  }
}
