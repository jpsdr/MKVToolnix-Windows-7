/** MPEG video helper functions (MPEG 1, 2 and 4)

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   \file

   \author Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/bit_reader.h"
#include "common/debugging.h"
#include "common/endian.h"
#include "common/memory_slice_cursor.h"
#include "common/strings/formatting.h"
#include "common/vc1.h"

namespace mtx::vc1 {

namespace {
debugging_option_c s_debug{"vc1"};
}

sequence_header_t::sequence_header_t() {
  memset(this, 0, sizeof(sequence_header_t));
}

entrypoint_t::entrypoint_t() {
  memset(this, 0, sizeof(entrypoint_t));
}

frame_header_t::frame_header_t() {
  init();
}

void
frame_header_t::init() {
  memset(this, 0, sizeof(frame_header_t));
}

frame_t::frame_t(frame_header_t const &p_header)
  : header{p_header}
{
  init();
}

void
frame_t::init() {
  data.reset();
  timestamp             = -1;
  duration             = 0;
  contains_field       = false;
  contains_entry_point = false;
}

bool
frame_t::is_key()
  const {
  return contains_entry_point || (header.frame_type == FRAME_TYPE_I);
}

bool
parse_sequence_header(const uint8_t *buf,
                      int size,
                      sequence_header_t &seqhdr) {
  try {
    static const struct { int n, d; } s_aspect_ratios[13] = {
      {   1,  1 },
      {  12, 11 },
      {  10, 11 },
      {  16, 11 },
      {  40, 33 },
      {  24, 11 },
      {  20, 11 },
      {  32, 11 },
      {  80, 33 },
      {  18, 11 },
      {  15, 11 },
      {  64, 33 },
      { 160, 99 }
    };

    static const int s_framerate_nr[5] = { 24, 25, 30, 50, 60 };
    static const int s_framerate_dr[2] = { 1000, 1001 };

    mtx::bits::reader_c bc(buf, size);
    sequence_header_t hdr;

    bc.skip_bits(32);           // Marker
    hdr.profile = bc.get_bits(2);

    if (PROFILE_ADVANCED != hdr.profile)
      return false;

    hdr.level              = bc.get_bits(3);
    hdr.chroma_format      = bc.get_bits(2);
    hdr.frame_rtq_postproc = bc.get_bits(3);
    hdr.bit_rtq_postproc   = bc.get_bits(5);
    hdr.postproc_flag      = bc.get_bit();
    hdr.pixel_width        = (bc.get_bits(12) + 1) << 1;
    hdr.pixel_height       = (bc.get_bits(12) + 1) << 1;
    hdr.pulldown_flag      = bc.get_bit();
    hdr.interlace_flag     = bc.get_bit();
    hdr.tf_counter_flag    = bc.get_bit();
    hdr.f_inter_p_flag     = bc.get_bit();
    bc.skip_bits(1);            // reserved
    hdr.psf_mode_flag      = bc.get_bit();
    hdr.display_info_flag  = bc.get_bit();

    if (hdr.display_info_flag) {
      hdr.display_width     = bc.get_bits(14) + 1;
      hdr.display_height    = bc.get_bits(14) + 1;
      hdr.aspect_ratio_flag = bc.get_bit();

      if (hdr.aspect_ratio_flag) {
        int aspect_ratio_idx      = bc.get_bits(4);

        if ((0 < aspect_ratio_idx) && (14 > aspect_ratio_idx)) {
          hdr.aspect_ratio_width  = s_aspect_ratios[aspect_ratio_idx - 1].n;
          hdr.aspect_ratio_height = s_aspect_ratios[aspect_ratio_idx - 1].d;

        } else if (15 == aspect_ratio_idx) {
          hdr.aspect_ratio_width  = bc.get_bits(8);
          hdr.aspect_ratio_height = bc.get_bits(8);

          if ((0 == hdr.aspect_ratio_width) || (0 == hdr.aspect_ratio_height))
            hdr.aspect_ratio_flag = false;

        } else
          hdr.aspect_ratio_flag   = false;
      }

      hdr.framerate_flag = bc.get_bit();
      if (hdr.framerate_flag) {
        if (bc.get_bit()) {
          hdr.framerate_num = 32;
          hdr.framerate_den = bc.get_bits(16) + 1;

        } else {
          int nr = bc.get_bits(8);
          int dr = bc.get_bits(4);

          if ((0 != nr) && (8 > nr) && (0 != dr) && (3 > dr)) {
            hdr.framerate_num = s_framerate_dr[dr - 1];
            hdr.framerate_den = s_framerate_nr[nr - 1] * 1000;

          } else
            hdr.framerate_flag = false;
        }
      }

      if (bc.get_bit()) {
        hdr.color_prim    = bc.get_bits(8);
        hdr.transfer_char = bc.get_bits(8);
        hdr.matrix_coef   = bc.get_bits(8);
      }
    }

    hdr.hrd_param_flag = bc.get_bit();
    if (hdr.hrd_param_flag) {
      hdr.hrd_num_leaky_buckets = bc.get_bits(5);
      bc.skip_bits(4 + 4);       // bitrate exponent, buffer size exponent
      bc.skip_bits(hdr.hrd_num_leaky_buckets * (16 + 16)); // hrd_rate, hrd_buffer
    }

    memcpy(&seqhdr, &hdr, sizeof(sequence_header_t));

    return true;
  } catch (...) {
    return false;
  }
}

bool
parse_entrypoint(const uint8_t *buf,
                 int size,
                 entrypoint_t &entrypoint,
                 sequence_header_t &seqhdr) {
  try {
    mtx::bits::reader_c bc(buf, size);
    entrypoint_t ep;

    bc.skip_bits(32);           // marker
    ep.broken_link_flag  = bc.get_bit();
    ep.closed_entry_flag = bc.get_bit();
    ep.pan_scan_flag     = bc.get_bit();
    ep.refdist_flag      = bc.get_bit();
    ep.loop_filter_flag  = bc.get_bit();
    ep.fast_uvmc_flag    = bc.get_bit();
    ep.extended_mv_flag  = bc.get_bit();
    ep.dquant            = bc.get_bits(2);
    ep.vs_transform_flag = bc.get_bit();
    ep.overlap_flag      = bc.get_bit();
    ep.quantizer_mode    = bc.get_bits(2);

    if (seqhdr.hrd_param_flag)
      bc.skip_bits(seqhdr.hrd_num_leaky_buckets * 8);

    ep.coded_dimensions_flag = bc.get_bit();
    if (ep.coded_dimensions_flag) {
      ep.coded_width  = (bc.get_bits(12) + 1) << 1;
      ep.coded_height = (bc.get_bits(12) + 1) << 1;
    }

    if (ep.extended_mv_flag)
      ep.extended_dmv_flag = bc.get_bit();

    ep.luma_scaling_flag = bc.get_bit();
    if (ep.luma_scaling_flag)
      ep.luma_scaling = bc.get_bits(3);

    ep.chroma_scaling_flag = bc.get_bit();
    if (ep.chroma_scaling_flag)
      ep.chroma_scaling = bc.get_bits(3);

    memcpy(&entrypoint, &ep, sizeof(entrypoint_t));

    return true;
  } catch (...) {
    return false;
  }
}

bool
parse_frame_header(const uint8_t *buf,
                   int size,
                   frame_header_t &frame_header,
                   sequence_header_t &seqhdr) {
  try {
    mtx::bits::reader_c bc(buf, size);
    frame_header_t fh;

    bc.skip_bits(32);           // marker

    auto field_mode = false;

    if (seqhdr.interlace_flag) {
      fh.fcm     = bc.get_012();
      field_mode = fh.fcm == FCM_ILACE_FIELD;
    }

    if (field_mode) {
      // Only set frame type to I for actual I/I fields. This is only
      // used for key frame determination, so gloss over the actual
      // field type differences.
      auto type     = bc.get_bits(3);
      fh.frame_type = 0 == type ? FRAME_TYPE_I : FRAME_TYPE_P;

    } else {
      auto type = bc.get_unary(false, 4);
      if (type >= 4) {
        fh.frame_type = FRAME_TYPE_P_SKIPPED;
        memcpy(&frame_header, &fh, sizeof(frame_header_t));

        return true;
      }

      static frame_type_e s_type_map[4] = { FRAME_TYPE_P, FRAME_TYPE_B, FRAME_TYPE_I, FRAME_TYPE_BI };
      fh.frame_type = s_type_map[type];
    }

    if (seqhdr.tf_counter_flag)
      fh.tf_counter = bc.get_bits(8);

    if (seqhdr.pulldown_flag) {
      if (!seqhdr.interlace_flag || seqhdr.psf_mode_flag)
        fh.repeat_frame            = bc.get_bits(2);
      else {
        fh.top_field_first_flag    = bc.get_bit();
        fh.repeat_first_field_flag = bc.get_bit();
      }
    }

    memcpy(&frame_header, &fh, sizeof(frame_header_t));

    return true;
  } catch (...) {
    return false;
  }
}

//
//  -------------------------------------------------
//

es_parser_c::es_parser_c()
  : m_stream_pos(0)
  , m_seqhdr_found(false)
  , m_seqhdr_changed{false}
  , m_previous_timestamp(0)
  , m_num_timestamps(0)
  , m_num_repeated_fields(0)
  , m_default_duration_forced(false)
  , m_default_duration(1000000000ll / 25)
{
}

void
es_parser_c::add_bytes(uint8_t *buffer,
                       int size) {
  mtx::mem::slice_cursor_c cursor;

  int previous_pos            = -1;
  int64_t previous_stream_pos = m_stream_pos;

  if (m_unparsed_buffer && (0 != m_unparsed_buffer->get_size()))
    cursor.add_slice(m_unparsed_buffer);
  cursor.add_slice(buffer, size);

  if (3 <= cursor.get_remaining_size()) {
    uint32_t marker = (1 << 24) | ((unsigned int)cursor.get_char() << 16) | ((unsigned int)cursor.get_char() << 8) | (unsigned int)cursor.get_char();

    while (true) {
      if (is_marker(marker)) {
        if (-1 != previous_pos) {
          int new_size = cursor.get_position() - 4 - previous_pos;

          memory_cptr packet(memory_c::alloc(new_size));
          cursor.copy(packet->get_buffer(), previous_pos, new_size);

          handle_packet(packet);
        }

        previous_pos = cursor.get_position() - 4;
        m_stream_pos = previous_stream_pos + previous_pos;
      }

      if (!cursor.char_available())
        break;

      marker <<= 8;
      marker  |= (unsigned int)cursor.get_char();
    }
  }

  if (-1 == previous_pos)
    previous_pos = 0;

  int new_size = cursor.get_size() - previous_pos;
  if (0 != new_size) {
    memory_cptr new_unparsed_buffer = memory_c::alloc(new_size);
    cursor.copy(new_unparsed_buffer->get_buffer(), previous_pos, new_size);
    m_unparsed_buffer = new_unparsed_buffer;

  } else
    m_unparsed_buffer.reset();
}

void
es_parser_c::flush() {
  if (m_unparsed_buffer && (4 <= m_unparsed_buffer->get_size())) {
    uint32_t marker = get_uint32_be(m_unparsed_buffer->get_buffer());
    if (is_marker(marker))
      handle_packet(memory_c::clone(m_unparsed_buffer->get_buffer(), m_unparsed_buffer->get_size()));
  }

  m_unparsed_buffer.reset();

  flush_frame();
}

void
es_parser_c::handle_packet(memory_cptr const &packet) {
  uint32_t marker = get_uint32_be(packet->get_buffer());

  switch (marker) {
    case MARKER_SEQHDR:
      handle_sequence_header_packet(packet);
      break;

    case MARKER_ENTRYPOINT:
      handle_entrypoint_packet(packet);
      break;

    case MARKER_FRAME:
      handle_frame_packet(packet);
      break;

    case MARKER_SLICE:
      handle_slice_packet(packet);
      break;

    case MARKER_FIELD:
      handle_field_packet(packet);
      break;

    case MARKER_ENDOFSEQ:
      handle_end_of_sequence_packet(packet);
      break;

    default:
      handle_unknown_packet(marker, packet);
      break;
  }
}

void
es_parser_c::handle_end_of_sequence_packet(memory_cptr const &) {
}

void
es_parser_c::handle_entrypoint_packet(memory_cptr const &packet) {
  if (!postpone_processing(packet))
    add_pre_frame_extra_data(packet);

  if (!m_raw_entrypoint)
    m_raw_entrypoint = packet->clone();
}

void
es_parser_c::handle_field_packet(memory_cptr const &packet) {
  if (postpone_processing(packet))
    return;

  add_post_frame_extra_data(packet);
}

void
es_parser_c::handle_frame_packet(memory_cptr const &packet) {
  if (postpone_processing(packet))
    return;

  flush_frame();

  frame_header_t frame_header;
  if (!parse_frame_header(packet->get_buffer(), packet->get_size(), frame_header, m_seqhdr))
    return;

  m_current_frame        = std::make_shared<frame_t>(frame_header);
  m_current_frame->data  = packet;
  m_current_frame->data->take_ownership();

  if (!m_timestamps.empty())
    mxdebug_if(s_debug, fmt::format("es_parser_c::handle_frame_packet: next provided timestamp {0} next calculated timestamp {1}\n", mtx::string::format_timestamp(m_timestamps.front()), mtx::string::format_timestamp(peek_next_calculated_timestamp())));

}

void
es_parser_c::handle_sequence_header_packet(memory_cptr const &packet) {
  flush_frame();

  add_pre_frame_extra_data(packet);

  sequence_header_t seqhdr;
  if (!parse_sequence_header(packet->get_buffer(), packet->get_size(), seqhdr))
    return;

  m_seqhdr_changed = !m_seqhdr_found || (packet->get_size() != m_raw_seqhdr->get_size()) || memcmp(packet->get_buffer(), m_raw_seqhdr->get_buffer(), packet->get_size());

  memcpy(&m_seqhdr, &seqhdr, sizeof(sequence_header_t));
  m_raw_seqhdr   = packet->clone();
  m_seqhdr_found = true;

  if (!m_default_duration_forced && m_seqhdr.framerate_flag && (0 != m_seqhdr.framerate_num) && (0 != m_seqhdr.framerate_den))
    m_default_duration = 1000000000ll * m_seqhdr.framerate_num / m_seqhdr.framerate_den;

  process_unparsed_packets();
}

void
es_parser_c::handle_slice_packet(memory_cptr const &packet) {
  if (postpone_processing(packet))
    return;

  add_post_frame_extra_data(packet);
}

void
es_parser_c::handle_unknown_packet(uint32_t,
                                   memory_cptr const &packet) {
  if (postpone_processing(packet))
    return;

  add_post_frame_extra_data(packet);
}

void
es_parser_c::flush_frame() {
  if (!m_current_frame)
    return;

  if (!m_pre_frame_extra_data.empty() || !m_post_frame_extra_data.empty())
    combine_extra_data_with_packet();

  m_current_frame->timestamp = get_next_timestamp();
  m_current_frame->duration = get_default_duration();

  m_frames.push_back(m_current_frame);

  m_current_frame.reset();
}

bool
es_parser_c::postpone_processing(memory_cptr const &packet) {
  if (m_seqhdr_found)
    return false;

  packet->take_ownership();
  m_unparsed_packets.push_back(packet);
  return true;
}

void
es_parser_c::process_unparsed_packets() {
  for (auto &packet : m_unparsed_packets)
    handle_frame_packet(packet);

  m_unparsed_packets.clear();
}

void
es_parser_c::combine_extra_data_with_packet() {
  auto sum_fn     = [](size_t size, const memory_cptr &buffer) { return size + buffer->get_size(); };
  auto extra_size = std::accumulate(m_pre_frame_extra_data.begin(), m_pre_frame_extra_data.end(), 0, sum_fn) + std::accumulate(m_post_frame_extra_data.begin(), m_post_frame_extra_data.end(), 0, sum_fn);

  auto new_packet = memory_c::alloc(extra_size + m_current_frame->data->get_size());
  auto ptr        = new_packet->get_buffer();

  for (const auto &mem : m_pre_frame_extra_data) {
    memcpy(ptr, mem->get_buffer(), mem->get_size());
    ptr += mem->get_size();

    if (get_uint32_be(mem->get_buffer()) == MARKER_ENTRYPOINT)
      m_current_frame->contains_entry_point = true;
  }

  memcpy(ptr, m_current_frame->data->get_buffer(), m_current_frame->data->get_size());
  ptr += m_current_frame->data->get_size();

  for (const auto &mem : m_post_frame_extra_data) {
    memcpy(ptr, mem->get_buffer(), mem->get_size());
    ptr += mem->get_size();

    if (MARKER_FIELD == get_uint32_be(mem->get_buffer()))
      m_current_frame->contains_field = true;
  }

  m_pre_frame_extra_data.clear();
  m_post_frame_extra_data.clear();

  m_current_frame->data = new_packet;
}

int64_t
es_parser_c::get_next_timestamp() {
  int64_t next_timestamp = m_previous_timestamp
                        + (m_num_timestamps + m_num_repeated_fields) * m_default_duration
                        - m_num_repeated_fields                     * m_default_duration / 2;

  if (is_timestamp_available()) {
    mxdebug_if(s_debug, fmt::format("\nes_parser_c::get_next_timestamp(): provided timestamp available; original next {0}, provided {1}\n", mtx::string::format_timestamp(next_timestamp), mtx::string::format_timestamp(m_timestamps.front())));

    next_timestamp         = m_timestamps.front();
    m_previous_timestamp   = m_timestamps.front();
    m_num_timestamps       = 0;
    m_num_repeated_fields = 0;

    m_timestamps.pop_front();
    m_timestamp_positions.pop_front();
  }

  m_num_timestamps += 1 + m_current_frame->header.repeat_frame;
  if (m_seqhdr.interlace_flag && m_current_frame->header.repeat_first_field_flag && !m_current_frame->contains_field)
    ++m_num_repeated_fields;

  return next_timestamp;
}

int64_t
es_parser_c::peek_next_calculated_timestamp()
  const {
  return m_previous_timestamp
    + (m_num_timestamps + m_num_repeated_fields) * m_default_duration
    - m_num_repeated_fields                     * m_default_duration / 2;
}

void
es_parser_c::add_pre_frame_extra_data(memory_cptr const &packet) {
  add_extra_data_if_not_present(m_pre_frame_extra_data, packet);
}

void
es_parser_c::add_post_frame_extra_data(memory_cptr const &packet) {
  add_extra_data_if_not_present(m_post_frame_extra_data, packet);
}

void
es_parser_c::add_extra_data_if_not_present(std::deque<memory_cptr> &extra_data,
                                           memory_cptr const &packet) {
  for (auto const &existing_packet : extra_data)
    if (*existing_packet == *packet)
      return;

  extra_data.push_back(packet->clone());
}

void
es_parser_c::add_timestamp(int64_t timestamp,
                          int64_t position) {
  position += m_stream_pos;
  if (m_unparsed_buffer)
    position += m_unparsed_buffer->get_size();

  m_timestamps.push_back(timestamp);
  m_timestamp_positions.push_back(position);
}

bool
es_parser_c::is_timestamp_available()
  const {
  return !m_timestamps.empty() && (m_timestamp_positions.front() <= m_stream_pos);
}

}
