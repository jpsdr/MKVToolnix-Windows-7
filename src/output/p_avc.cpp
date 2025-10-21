/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   MPEG4 part 10 video output module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <cmath>

#include "common/avc/es_parser.h"
#include "common/codec.h"
#include "common/endian.h"
#include "common/hacks.h"
#include "common/list_utils.h"
#include "common/mpeg.h"
#include "common/strings/formatting.h"
#include "merge/output_control.h"
#include "output/p_avc.h"

avc_video_packetizer_c::
avc_video_packetizer_c(generic_reader_c *p_reader,
                       track_info_c &p_ti,
                       int64_t default_duration,
                       int width,
                       int height)
  : generic_video_packetizer_c{p_reader, p_ti, MKV_V_MPEG4_AVC, default_duration, width, height}
  , m_debug_fix_bistream_timing_info{"fix_bitstream_timing_info"}
{
  m_relaxed_timestamp_checking = true;

  if (5 <= m_ti.m_private_data->get_size())
    m_nalu_size_len = (m_ti.m_private_data->get_buffer()[4] & 0x03) + 1;

  set_codec_private(m_ti.m_private_data);
}

void
avc_video_packetizer_c::set_headers() {
  if (m_ti.m_private_data && m_ti.m_private_data->get_size())
    extract_aspect_ratio();

  int64_t divisor = 2;

  if (m_default_duration_forced) {
    if (m_htrack_default_duration_indicates_fields) {
      set_track_default_duration(m_htrack_default_duration / 2, true);
      divisor = 1;
    }

    m_track_default_duration = m_htrack_default_duration;
  }

  if ((-1 == m_track_default_duration) && m_timestamp_factory)
    m_track_default_duration = m_timestamp_factory->get_default_duration(-1);

  if ((-1 == m_track_default_duration) && (0 < m_default_duration))
    m_track_default_duration = m_default_duration;

  if (-1 != m_track_default_duration)
    m_track_default_duration /= divisor;

  mxdebug_if(m_debug_fix_bistream_timing_info,
             fmt::format("fix_bitstream_timing_info [AVCC]: factory default_duration {0} default_duration_forced? {1} htrack_default_duration {2} default_duration {3} m_track_default_duration {4}\n",
                         m_timestamp_factory ? m_timestamp_factory->get_default_duration(-1) : -2,
                         m_default_duration_forced, m_htrack_default_duration,
                         m_default_duration, m_track_default_duration));

  if (   m_ti.m_private_data
      && m_ti.m_private_data->get_size()
      && m_ti.m_fix_bitstream_frame_rate
      && (-1 != m_track_default_duration))
    set_codec_private(mtx::avc::fix_sps_fps(m_ti.m_private_data, m_track_default_duration));

  generic_video_packetizer_c::set_headers();
}

void
avc_video_packetizer_c::extract_aspect_ratio() {
  auto result = mtx::avc::extract_par(m_ti.m_private_data);

  set_codec_private(result.new_avcc);

  if (!result.is_valid() || display_dimensions_or_aspect_ratio_set())
    return;

  auto par = mtx::rational(result.numerator, result.denominator);

  set_video_display_dimensions(1 <= par ? mtx::to_int_rounded(m_width * par) : m_width,
                               1 <= par ? m_height                           : mtx::to_int_rounded(m_height / par),
                               generic_packetizer_c::ddu_pixels,
                               option_source_e::bitstream);

  mxinfo_tid(m_ti.m_fname, m_ti.m_id,
             fmt::format(FY("Extracted the aspect ratio information from the MPEG-4 layer 10 (AVC) video data and set the display dimensions to {0}/{1}.\n"),
                         m_ti.m_display_width, m_ti.m_display_height));
}

void
avc_video_packetizer_c::process_impl(packet_cptr const &packet) {
  if (VFT_PFRAMEAUTOMATIC == packet->bref) {
    packet->fref = -1;
    packet->bref = m_ref_timestamp;
  }

  m_ref_timestamp = packet->timestamp;

  process_nalus(*packet->data);

  add_packet(packet);
}

connection_result_e
avc_video_packetizer_c::can_connect_to(generic_packetizer_c *src,
                                       std::string &error_message) {
  avc_video_packetizer_c *vsrc = dynamic_cast<avc_video_packetizer_c *>(src);
  if (!vsrc)
    return CAN_CONNECT_NO_FORMAT;

  if (m_ti.m_private_data && vsrc->m_ti.m_private_data && memcmp(m_ti.m_private_data->get_buffer(), vsrc->m_ti.m_private_data->get_buffer(), m_ti.m_private_data->get_size())) {
    error_message = fmt::format(FY("The codec's private data does not match. Both have the same length ({0}) but different content."), m_ti.m_private_data->get_size());
    return CAN_CONNECT_MAYBE_CODECPRIVATE;
  }

  return CAN_CONNECT_YES;
}

void
avc_video_packetizer_c::process_nalus(memory_c &data)
  const {
  auto ptr        = data.get_buffer();
  auto total_size = data.get_size();
  auto idx        = 0u;

  while ((idx + m_nalu_size_len) < total_size) {
    auto nalu_size = get_uint_be(&ptr[idx], m_nalu_size_len) + m_nalu_size_len;

    if ((idx + nalu_size) > total_size)
      break;

    if (static_cast<int>(nalu_size) < m_nalu_size_len)
      break;

    auto const nalu_type = ptr[idx + m_nalu_size_len] & 0x1f;

    if (   (static_cast<int>(nalu_size) == m_nalu_size_len) // empty NALU?
        || mtx::included_in(nalu_type, mtx::avc::NALU_TYPE_FILLER_DATA, mtx::avc::NALU_TYPE_ACCESS_UNIT)) {
      memory_c::splice(data, idx, nalu_size);
      total_size -= nalu_size;
      ptr         = data.get_buffer();
      continue;
    }

    if (   (nalu_type == mtx::avc::NALU_TYPE_SEQ_PARAM)
        && m_ti.m_fix_bitstream_frame_rate
        && (-1 != m_track_default_duration)) {
      mxdebug_if(m_debug_fix_bistream_timing_info, fmt::format("fix_bitstream_timing_info [NALU]: m_track_default_duration {0}\n", m_track_default_duration));

      mtx::avc::sps_info_t sps_info;
      auto parsed_nalu = mtx::avc::parse_sps(mtx::mpeg::nalu_to_rbsp(memory_c::clone(&ptr[idx + m_nalu_size_len], nalu_size - m_nalu_size_len)), sps_info, true, true, m_track_default_duration);

      if (parsed_nalu) {
        parsed_nalu = mtx::mpeg::rbsp_to_nalu(parsed_nalu);

        memory_c::splice(data, idx + m_nalu_size_len, nalu_size - m_nalu_size_len, *parsed_nalu);

        total_size = data.get_size();
        ptr        = data.get_buffer();

        put_uint_be(&ptr[idx], parsed_nalu->get_size(), m_nalu_size_len);

        idx += parsed_nalu->get_size() + m_nalu_size_len;
        continue;
      }
    }

    idx += nalu_size;
  }

  // empty NALU at the end?
  if (total_size == (idx + m_nalu_size_len))
    data.set_size(idx);
}
