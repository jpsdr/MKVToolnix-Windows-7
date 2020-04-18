/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   HEVC video output module

*/

#include "common/common_pch.h"

#include <cmath>

#include "common/codec.h"
#include "common/endian.h"
#include "common/hacks.h"
#include "common/hevc.h"
#include "common/hevc_es_parser.h"
#include "common/strings/formatting.h"
#include "merge/output_control.h"
#include "output/p_hevc.h"

class hevc_video_packetizer_private_c {
public:
  int nalu_size_len_src{}, nalu_size_len_dst{};
  int64_t max_nalu_size{};
  bool rederive_timestamp_order{};
  std::unique_ptr<mtx::hevc::es_parser_c> parser;
};

hevc_video_packetizer_c::
hevc_video_packetizer_c(generic_reader_c *p_reader,
                        track_info_c &p_ti,
                        double fps,
                        int width,
                        int height)
  : generic_video_packetizer_c{p_reader, p_ti, MKV_V_MPEGH_HEVC, fps, width, height}
  , p_ptr{new hevc_video_packetizer_private_c}
{
  m_relaxed_timestamp_checking = true;

  setup_nalu_size_len_change();

  set_codec_private(m_ti.m_private_data);
}

void
hevc_video_packetizer_c::set_headers() {
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

  auto par = static_cast<double>(result.numerator) / static_cast<double>(result.denominator);

  set_video_display_dimensions(1 <= par ? std::llround(m_width * par) : m_width,
                               1 <= par ? m_height                    : std::llround(m_height / par),
                               generic_packetizer_c::ddu_pixels,
                               OPTION_SOURCE_BITSTREAM);

  mxinfo_tid(m_ti.m_fname, m_ti.m_id,
             fmt::format(Y("Extracted the aspect ratio information from the HEVC video data and set the display dimensions to {0}/{1}.\n"),
                         m_ti.m_display_width, m_ti.m_display_height));
}

int
hevc_video_packetizer_c::process(packet_cptr packet) {
  auto &p = *p_func();

  if (p.rederive_timestamp_order) {
    process_rederiving_timestamp_order(*packet);
    return FILE_STATUS_MOREDATA;
  }

  if (VFT_PFRAMEAUTOMATIC == packet->bref) {
    packet->fref = -1;
    packet->bref = m_ref_timestamp;
  }

  m_ref_timestamp = packet->timestamp;

  if (p.nalu_size_len_dst && (p.nalu_size_len_dst != p.nalu_size_len_src))
    change_nalu_size_len(packet);

  add_packet(packet);

  return FILE_STATUS_MOREDATA;
}

connection_result_e
hevc_video_packetizer_c::can_connect_to(generic_packetizer_c *src,
                                        std::string &error_message) {
  hevc_video_packetizer_c *vsrc = dynamic_cast<hevc_video_packetizer_c *>(src);
  if (!vsrc)
    return CAN_CONNECT_NO_FORMAT;

  if (m_ti.m_private_data && vsrc->m_ti.m_private_data && memcmp(m_ti.m_private_data->get_buffer(), vsrc->m_ti.m_private_data->get_buffer(), m_ti.m_private_data->get_size())) {
    error_message = fmt::format(Y("The codec's private data does not match. Both have the same length ({0}) but different content."), m_ti.m_private_data->get_size());
    return CAN_CONNECT_MAYBE_CODECPRIVATE;
  }

  return CAN_CONNECT_YES;
}

void
hevc_video_packetizer_c::setup_nalu_size_len_change() {
  if (!m_ti.m_private_data || (23 > m_ti.m_private_data->get_size()))
    return;

  auto &p             = *p_func();
  auto private_data   = m_ti.m_private_data->get_buffer();
  auto const len_pos  = 21;
  p.nalu_size_len_src = (private_data[len_pos] & 0x03) + 1;
  p.nalu_size_len_dst = p.nalu_size_len_src;

  if (!m_ti.m_nalu_size_length || (m_ti.m_nalu_size_length == p.nalu_size_len_src))
    return;

  p.nalu_size_len_dst   = m_ti.m_nalu_size_length;
  private_data[len_pos] = (private_data[len_pos] & 0xfc) | (p.nalu_size_len_dst - 1);
  p.max_nalu_size       = 1ll << (8 * p.nalu_size_len_dst);

  set_codec_private(m_ti.m_private_data);

  mxverb(2, fmt::format("HEVC: Adjusting NALU size length from {0} to {1}\n", p.nalu_size_len_src, p.nalu_size_len_dst));
}

void
hevc_video_packetizer_c::change_nalu_size_len(packet_cptr const &packet) {
  unsigned char *src = packet->data->get_buffer();
  int size           = packet->data->get_size();

  if (!src || !size)
    return;

  std::vector<int> nalu_sizes;

  auto &p     = *p_func();
  int src_pos = 0;

  // Find all NALU sizes in this packet.
  while (src_pos < size) {
    if ((size - src_pos) < p.nalu_size_len_src)
      break;

    int nalu_size = 0;
    int i;
    for (i = 0; i < p.nalu_size_len_src; ++i)
      nalu_size = (nalu_size << 8) + src[src_pos + i];

    if ((size - src_pos - p.nalu_size_len_src) < nalu_size)
      nalu_size = size - src_pos - p.nalu_size_len_src;

    if (nalu_size > p.max_nalu_size)
      mxerror_tid(m_ti.m_fname, m_ti.m_id, fmt::format(Y("The chosen NALU size length of {0} is too small. Try using '4'.\n"), p.nalu_size_len_dst));

    src_pos += p.nalu_size_len_src + nalu_size;

    nalu_sizes.push_back(nalu_size);
  }

  // Allocate memory if the new NALU size length is greater
  // than the previous one. Otherwise reuse the existing memory.
  if (p.nalu_size_len_dst > p.nalu_size_len_src) {
    int new_size = size + nalu_sizes.size() * (p.nalu_size_len_dst - p.nalu_size_len_src);
    packet->data = memory_c::alloc(new_size);
  }

  // Copy the NALUs and write the new sized length field.
  unsigned char *dst = packet->data->get_buffer();
  src_pos            = 0;
  int dst_pos        = 0;

  size_t i;
  for (i = 0; nalu_sizes.size() > i; ++i) {
    int nalu_size = nalu_sizes[i];

    int shift;
    for (shift = 0; shift < p.nalu_size_len_dst; ++shift)
      dst[dst_pos + shift] = (nalu_size >> (8 * (p.nalu_size_len_dst - 1 - shift))) & 0xff;

    memmove(&dst[dst_pos + p.nalu_size_len_dst], &src[src_pos + p.nalu_size_len_src], nalu_size);

    src_pos += p.nalu_size_len_src + nalu_size;
    dst_pos += p.nalu_size_len_dst + nalu_size;
  }

  packet->data->set_size(dst_pos);
}

void
hevc_video_packetizer_c::rederive_timestamp_order() {
  auto &p = *p_func();

  if (p.rederive_timestamp_order)
    return;

  p.rederive_timestamp_order = true;
  p.parser.reset(new mtx::hevc::es_parser_c);

  p.parser->set_hevcc(m_hcodec_private);
  p.parser->set_container_default_duration(m_htrack_default_duration);
}

void
hevc_video_packetizer_c::process_rederiving_timestamp_order(packet_t &packet) {
  auto &p = *p_func();

  if (packet.is_key_frame() && (VFT_PFRAMEAUTOMATIC != packet.bref))
    p.parser->set_next_i_slice_is_key_frame();

  if (packet.has_timestamp())
    p.parser->add_timestamp(packet.timestamp);

  p.parser->add_bytes_framed(packet.data, p.nalu_size_len_src);

  flush_frames();
}

void
hevc_video_packetizer_c::flush_impl() {
  auto &p = *p_func();

  if (!p.rederive_timestamp_order)
    return;

  p.parser->flush();
  flush_frames();
}

void
hevc_video_packetizer_c::flush_frames() {
  auto &p = *p_func();

  if (!p.rederive_timestamp_order)
    return;

  while (p.parser->frame_available()) {
    auto frame = p.parser->get_frame();
    add_packet(new packet_t(frame.m_data, frame.m_start,
                            frame.m_end > frame.m_start ? frame.m_end - frame.m_start : m_htrack_default_duration,
                            frame.m_keyframe            ? -1                          : frame.m_start + frame.m_ref1));
  }
}
