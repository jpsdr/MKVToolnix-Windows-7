/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   MPEG1/2 video output module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/codec.h"
#include "common/endian.h"
#include "common/hacks.h"
#include "common/mpeg1_2.h"
#include "common/mpeg4_p2.h"
#include "common/strings/formatting.h"
#include "merge/output_control.h"
#include "output/p_mpeg1_2.h"

mpeg1_2_video_packetizer_c::
mpeg1_2_video_packetizer_c(generic_reader_c *p_reader,
                           track_info_c &p_ti,
                           int version,
                           int64_t default_duration,
                           int width,
                           int height,
                           int dwidth,
                           int dheight,
                           bool framed)
  : generic_video_packetizer_c{p_reader, p_ti, "V_MPEG1", default_duration, width, height}
  , m_framed{framed}
  , m_aspect_ratio_extracted{true}
  , m_num_removed_stuffing_bytes{}
  , m_debug_stuffing_removal{"mpeg1_2|mpeg1_2_stuffing_removal"}
{

  set_codec_id(fmt::format("V_MPEG{0}", version));
  if (!display_dimensions_or_aspect_ratio_set()) {
    if ((0 < dwidth) && (0 < dheight))
      set_video_display_dimensions(dwidth, dheight, generic_packetizer_c::ddu_pixels, option_source_e::bitstream);
    else
      m_aspect_ratio_extracted = false;
  }

  m_timestamp_factory_application_mode = TFA_SHORT_QUEUEING;

  // m_parser.SeparateSequenceHeaders();
}

mpeg1_2_video_packetizer_c::~mpeg1_2_video_packetizer_c() {
  mxdebug_if(m_debug_stuffing_removal, fmt::format("Total number of stuffing bytes removed: {0}\n", m_num_removed_stuffing_bytes));
}

void
mpeg1_2_video_packetizer_c::remove_stuffing_bytes_and_handle_sequence_headers(packet_cptr const &packet) {
  mxdebug_if(m_debug_stuffing_removal, fmt::format("Starting stuff removal, frame size {0}, timestamp {1}\n", packet->data->get_size(), mtx::string::format_timestamp(packet->timestamp)));

  auto buf              = packet->data->get_buffer();
  auto size             = packet->data->get_size();
  size_t pos            = 4;
  size_t start_code_pos = 0;
  size_t stuffing_start = 0;
  auto marker           = get_uint32_be(buf);
  uint32_t chunk_type   = 0;
  bool seq_hdr_found    = false;

  // auto mid_remover      = [this, &stuffing_start, &pos, &size, &buf]() {
  //   if (!stuffing_start || (stuffing_start >= (pos - 4)))
  //     return;

  //   auto num_stuffing_bytes       = pos - 4 - stuffing_start;
  //   m_num_removed_stuffing_bytes += num_stuffing_bytes;

  //   if (m_debug_stuffing_removal)
  //     debugging_c::hexdump(&buf[stuffing_start], num_stuffing_bytes + 4);
  //   ::memmove(&buf[stuffing_start], &buf[pos - 4], size - pos + 4);

  //   pos  -= num_stuffing_bytes;
  //   size -= num_stuffing_bytes;

  //   mxdebug_if(m_debug_stuffing_removal, fmt::format("    Stuffing in the middle: {0}\n", num_stuffing_bytes));
  // };

  memory_cptr new_seq_hdr;
  auto seq_hdr_copier = [&chunk_type, &seq_hdr_found, &buf, &start_code_pos, &pos, &new_seq_hdr, &size](bool at_end) {
    if ((mtx::mpeg1_2::SEQUENCE_HEADER_START_CODE != chunk_type) && (mtx::mpeg1_2::EXT_START_CODE != chunk_type))
      return;

    // sequence extension and sequence display extension only
    if (   (mtx::mpeg1_2::EXT_START_CODE   == chunk_type)
        && ((buf[start_code_pos+4] & 0xf0) != 0x10)
        && ((buf[start_code_pos+4] & 0xf0) != 0x20))
      return;

    if (mtx::mpeg1_2::SEQUENCE_HEADER_START_CODE == chunk_type)
      seq_hdr_found = true;

    else if (!seq_hdr_found)
      return;

    auto bytes_to_copy = at_end ? pos - start_code_pos : pos - 4 - start_code_pos;

    if (!new_seq_hdr)
      new_seq_hdr = memory_c::clone(&buf[start_code_pos], bytes_to_copy);

    else
      new_seq_hdr->add(&buf[start_code_pos], bytes_to_copy);

    if (!mtx::hacks::is_engaged(mtx::hacks::USE_CODEC_STATE_ONLY))
      return;

    memmove(&buf[start_code_pos], &buf[pos - 4], size - pos + 4);
    pos  -= bytes_to_copy;
    size -= bytes_to_copy;
  };

  while (pos < size) {
    if ((mtx::mpeg1_2::SLICE_START_CODE_LOWER <= marker) && (mtx::mpeg1_2::SLICE_START_CODE_UPPER >= marker)) {
      mxdebug_if(m_debug_stuffing_removal, fmt::format("  Slice start code at {0}\n", pos - 4));

      // mid_remover();
      seq_hdr_copier(false);

      chunk_type     = mtx::mpeg1_2::SLICE_START_CODE_LOWER;
      stuffing_start = 0;
      start_code_pos = pos - 4;

    } else if ((marker & 0xffffff00) == 0x00000100) {
      mxdebug_if(m_debug_stuffing_removal, fmt::format("  Non-slice start code 0x{1:08x} at {0}\n", pos - 4, marker));

      // mid_remover();
      seq_hdr_copier(false);

      chunk_type     = marker;
      stuffing_start = 0;
      start_code_pos = pos - 4;

    } else if ((mtx::mpeg1_2::SLICE_START_CODE_LOWER == chunk_type) && !stuffing_start && (marker == 0x00000000)) {
      mxdebug_if(m_debug_stuffing_removal, fmt::format("    Start at {0}\n", pos - 3));

      stuffing_start = pos - 3;
    }

    marker <<= 8;
    marker  |= buf[pos];
    ++pos;
  }

  if ((marker & 0xffffff00) == 0x00000100) {
    seq_hdr_copier(false);
    // mid_remover();

  // } else if (stuffing_start && (stuffing_start < size)) {
  //   mxdebug_if(m_debug_stuffing_removal, fmt::format("    Stuffing at the end: chunk_type 0x{0:08x} stuffing_start {1} stuffing_size {2}\n", chunk_type, stuffing_start, stuffing_start ? size - stuffing_start : 0));

  //   m_num_removed_stuffing_bytes += size - stuffing_start;
  //   size                          = stuffing_start;
  }

  packet->data->set_size(size);

  if (!new_seq_hdr)
    return;

  if (!m_hcodec_private) {
    set_codec_private(new_seq_hdr);
    rerender_track_headers();
  }

  if (m_seq_hdr && (*new_seq_hdr != *m_seq_hdr)) {
    mxdebug_if(m_debug_stuffing_removal, fmt::format("Sequence header changed, adding a CodecState of {0} bytes\n", new_seq_hdr->get_size()));
    packet->codec_state = new_seq_hdr->clone();
  }

  m_seq_hdr = new_seq_hdr;
}

void
mpeg1_2_video_packetizer_c::process_impl(packet_cptr const &packet) {
  if (0 >= m_default_duration)
    extract_fps(packet->data->get_buffer(), packet->data->get_size());

  if (!m_aspect_ratio_extracted)
    extract_aspect_ratio(packet->data->get_buffer(), packet->data->get_size());

  if (m_framed)
    process_framed(packet);
  else
    process_unframed(packet);
}

void
mpeg1_2_video_packetizer_c::process_framed(packet_cptr const &packet) {
  if (0 == packet->data->get_size())
    return;

  if (4 <= packet->data->get_size())
    remove_stuffing_bytes_and_handle_sequence_headers(packet);

  generic_video_packetizer_c::process_impl(packet);
}

void
mpeg1_2_video_packetizer_c::process_unframed(packet_cptr const &packet) {
  int state = m_parser.GetState();
  if ((MPV_PARSER_STATE_EOS == state) || (MPV_PARSER_STATE_ERROR == state))
    return;

  auto old_memory = packet->data;
  auto data_ptr   = old_memory->get_buffer();
  int new_bytes   = old_memory->get_size();

  if (packet->has_timestamp())
    m_parser.AddTimestamp(packet->timestamp);

  do {
    int bytes_to_add = std::min<int>(m_parser.GetFreeBufferSpace(), new_bytes);
    if (0 < bytes_to_add) {
      m_parser.WriteData(data_ptr, bytes_to_add);
      data_ptr  += bytes_to_add;
      new_bytes -= bytes_to_add;
    }

    state = m_parser.GetState();
    while (MPV_PARSER_STATE_FRAME == state) {
      auto frame = std::shared_ptr<MPEGFrame>(m_parser.ReadFrame());
      if (!frame)
        break;

      packet_cptr new_packet  = packet_cptr(new packet_t(memory_c::take_ownership(frame->data, frame->size), frame->timestamp, frame->duration, frame->refs[0], frame->refs[1]));

      remove_stuffing_bytes_and_handle_sequence_headers(new_packet);

      generic_video_packetizer_c::process_impl(new_packet);

      frame->data = nullptr;
      state       = m_parser.GetState();
    }
  } while (0 < new_bytes);
}

void
mpeg1_2_video_packetizer_c::flush_impl() {
  m_parser.SetEOS();
  auto empty = ""s;
  generic_packetizer_c::process(std::make_shared<packet_t>(memory_c::borrow(empty)));
}

void
mpeg1_2_video_packetizer_c::extract_fps(const uint8_t *buffer,
                                        int size) {
  int idx = mtx::mpeg1_2::extract_fps_idx(buffer, size);
  if (0 > idx)
    return;

  auto fps = mtx::mpeg1_2::get_fps(idx);
  if (0 < fps) {
    m_default_duration = static_cast<int64_t>(1'000'000'000.0 / fps);
    set_track_default_duration(m_default_duration);
    rerender_track_headers();

  } else
    m_default_duration = 0;
}

void
mpeg1_2_video_packetizer_c::extract_aspect_ratio(const uint8_t *buffer,
                                                 int size) {
  if (display_dimensions_or_aspect_ratio_set())
    return;

  auto aspect_ratio = mtx::mpeg1_2::extract_aspect_ratio(buffer, size);

  if (!aspect_ratio)
    return;

  set_video_display_dimensions(1 == *aspect_ratio ? m_width : mtx::to_int(m_height * *aspect_ratio), m_height, generic_packetizer_c::ddu_pixels, option_source_e::bitstream);

  rerender_track_headers();
  m_aspect_ratio_extracted = true;
}
