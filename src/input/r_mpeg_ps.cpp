/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   MPEG PS (program stream) demultiplexer module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <QRegularExpression>

#include "common/ac3.h"
#include "common/bit_reader.h"
#include "common/common_urls.h"
#include "common/debugging.h"
#include "common/endian.h"
#include "common/error.h"
#include "common/id_info.h"
#include "common/mp3.h"
#include "common/mpeg.h"
#include "common/mpeg1_2.h"
#include "common/path.h"
#include "common/qt.h"
#include "common/strings/formatting.h"
#include "common/truehd.h"
#include "input/r_mpeg_ps.h"
#include "merge/file_status.h"
#include "mpegparser/M2VParser.h"
#include "output/p_ac3.h"
#include "output/p_avc_es.h"
#include "output/p_dts.h"
#include "output/p_mp3.h"
#include "output/p_mpeg1_2.h"
#include "output/p_pcm.h"
#include "output/p_truehd.h"
#include "output/p_vc1.h"

bool
mpeg_ps_reader_c::probe_file() {
  uint8_t buf[4];

  if (m_in->read(buf, 4) != 4)
    return false;

  if (get_uint32_be(buf) == mtx::mpeg1_2::PACKET_START_CODE)
    return true;

  m_in->setFilePointer(0);

  auto mem      = memory_c::alloc(32 * 1024);
  auto num_read = m_in->read(mem, mem->get_size());

  if (num_read < 4)
    return false;

  auto base                           = mem->get_buffer();
  auto code                           = get_uint32_be(base);
  auto offset                         = 2u;
  auto system_header_start_code_found = false;
  auto packet_start_code_found        = false;

  while(   ((offset + 4) < num_read)
        && (!system_header_start_code_found || !packet_start_code_found)) {
    ++offset;
    code = (code << 8) | base[offset];

    if (code == mtx::mpeg1_2::SYSTEM_HEADER_START_CODE)
      system_header_start_code_found = true;

    else if (code == mtx::mpeg1_2::PACKET_START_CODE)
      packet_start_code_found = true;
  }

  return system_header_start_code_found && packet_start_code_found;
}

void
mpeg_ps_reader_c::read_headers() {
  try {
    uint8_t byte;

    if (!m_ti.m_disable_multi_file && Q(mtx::fs::to_path(m_ti.m_fname).filename()).contains(QRegularExpression{"^vts_\\d+_\\d+", QRegularExpression::CaseInsensitiveOption})) {
      m_in.reset();               // Close the source file first before opening it a second time.
      m_in = mm_multi_file_io_c::open_multi(m_ti.m_fname, false);
    }

    m_size          = m_in->get_size();
    m_probe_range   = calculate_probe_range(m_size, 10 * 1024 * 1024);
    uint32_t header = m_in->read_uint32_be();
    bool done       = m_in->eof();
    version         = -1;

    while (!done) {
      uint8_t stream_id;
      uint16_t pes_packet_length;

      switch (header) {
        case mtx::mpeg1_2::PACKET_START_CODE:
          mxdebug_if(m_debug_headers, fmt::format("mpeg_ps: packet start at {0}\n", m_in->getFilePointer() - 4));

          if (-1 == version) {
            byte = m_in->read_uint8();
            if ((byte & 0xc0) != 0)
              version = 2;      // MPEG-2 PS
            else
              version = 1;
            m_in->skip(-1);
          }

          m_in->skip(2 * 4);   // pack header
          if (2 == version) {
            m_in->skip(1);
            byte = m_in->read_uint8() & 0x07;
            m_in->skip(byte);  // stuffing bytes
          }
          header = m_in->read_uint32_be();
          break;

        case mtx::mpeg1_2::SYSTEM_HEADER_START_CODE:
          mxdebug_if(m_debug_headers, fmt::format("mpeg_ps: system header start code at {0}\n", m_in->getFilePointer() - 4));

          m_in->skip(2 * 4);   // system header
          byte = m_in->read_uint8();
          while ((byte & 0x80) == 0x80) {
            m_in->skip(2);     // P-STD info
            byte = m_in->read_uint8();
          }
          m_in->skip(-1);
          header = m_in->read_uint32_be();
          break;

        case mtx::mpeg1_2::MPEG_PROGRAM_END_CODE:
          done = !resync_stream(header);
          break;

        case mtx::mpeg1_2::PROGRAM_STREAM_MAP_START_CODE:
          parse_program_stream_map();
          done = !resync_stream(header);
          break;

        default:
          if (!mtx::mpeg::is_start_code(header)) {
            mxdebug_if(m_debug_headers, fmt::format("mpeg_ps: unknown header 0x{0:08x} at {1}\n", header, m_in->getFilePointer() - 4));
            done = !resync_stream(header);
            break;
          }

          stream_id = header & 0xff;
          m_in->save_pos();
          found_new_stream(stream_id);
          m_in->restore_pos();
          pes_packet_length = m_in->read_uint16_be();

          mxdebug_if(m_debug_headers, fmt::format("mpeg_ps: id 0x{0:02x} len {1} at {2}\n", static_cast<unsigned int>(stream_id), pes_packet_length, m_in->getFilePointer() - 4 - 2));

          m_in->skip(pes_packet_length);

          header = m_in->read_uint32_be();

          break;
      }

      done |= m_in->eof() || (m_in->getFilePointer() >= m_probe_range);
    } // while (!done)

  } catch (...) {
  }

  sort_tracks();
  calculate_global_timestamp_offset();

  m_in->setFilePointer(0);

  if (verbose) {
    show_demuxer_info();
    auto multi_in = dynamic_cast<mm_multi_file_io_c *>(get_underlying_input());
    if (multi_in)
      multi_in->display_other_file_info();
  }
}

void
mpeg_ps_reader_c::sort_tracks() {
  size_t i;

  for (i = 0; tracks.size() > i; ++i)
    tracks[i]->sort_key = (  'v' == tracks[i]->type ? 0x00000
                           : 'a' == tracks[i]->type ? 0x10000
                           : 's' == tracks[i]->type ? 0x20000
                           :                          0x30000)
      + tracks[i]->id.idx();

  std::sort(tracks.begin(), tracks.end());

  for (i = 0; tracks.size() > i; ++i)
    id2idx[tracks[i]->id.idx()] = i;

  if (!m_debug_headers)
    return;

  auto info = "mpeg_ps: Supported streams, sorted by ID: "s;
  for (i = 0; tracks.size() > i; ++i)
    info += fmt::format("0x{0:02x}(0x{1:02x}) ", tracks[i]->id.id, tracks[i]->id.sub_id);

  mxdebug(info + "\n");
}

void
mpeg_ps_reader_c::calculate_global_timestamp_offset() {
  // Calculate by how much the timestamps have to be offset
  if (tracks.empty())
    return;

  for (auto &track : tracks)
    track->timestamp_offset -= track->timestamp_b_frame_offset;

  auto const &track_min_offset = *std::min_element(tracks.begin(), tracks.end(), [](mpeg_ps_track_ptr const &a, mpeg_ps_track_ptr const &b) { return a->timestamp_offset < b->timestamp_offset; });
  global_timestamp_offset      = std::numeric_limits<int64_t>::max() == track_min_offset->timestamp_offset ? 0 : track_min_offset->timestamp_offset;

  for (auto &track : tracks)
    track->timestamp_offset = std::numeric_limits<int64_t>::max() == track->timestamp_offset ? global_timestamp_offset : track->timestamp_offset - global_timestamp_offset;

  if (!m_debug_timestamps)
    return;

  std::string output = fmt::format("mpeg_ps: Timestamp offset: min was {0} ", global_timestamp_offset);
  for (auto const &track : tracks)
    output += fmt::format("{0}={1} ", track->id, track->timestamp_offset);
  mxdebug(output + "\n");
}

bool
mpeg_ps_reader_c::read_timestamp(int c,
                                 int64_t &timestamp) {
  int d = m_in->read_uint16_be();
  int e = m_in->read_uint16_be();

  if (((c & 1) != 1) || ((d & 1) != 1) || ((e & 1) != 1))
    return false;

  timestamp = (int64_t)((((c >> 1) & 7) << 30) | ((d >> 1) << 15) | (e >> 1)) * 100000ll / 9;

  return true;
}

bool
mpeg_ps_reader_c::read_timestamp(mtx::bits::reader_c &bc,
                                 int64_t &timestamp) {
  bc.skip_bits(4);
  int64_t temp_timestamp = bc.get_bits(3);
  if (1 != bc.get_bit())
    return false;
  temp_timestamp = (temp_timestamp << 15) | bc.get_bits(15);
  if (1 != bc.get_bit())
    return false;
  temp_timestamp = (temp_timestamp << 15) | bc.get_bits(15);
  if (1 != bc.get_bit())
    return false;

  timestamp = temp_timestamp * 100000ll / 9;

  return true;
}

void
mpeg_ps_reader_c::parse_program_stream_map() {
  int len     = 0;
  int64_t pos = m_in->getFilePointer();

  try {
    len = m_in->read_uint16_be();

    if (!len || (1018 < len))
      throw false;

    m_in->skip(2);

    int prog_len = m_in->read_uint16_be();
    m_in->skip(prog_len);

    int es_map_len = m_in->read_uint16_be();
    es_map_len     = std::min(es_map_len, len - prog_len - 8);

    while (4 <= es_map_len) {
      int type   = m_in->read_uint8();
      int id     = m_in->read_uint8();
      es_map[id] = type;

      int plen = m_in->read_uint16_be();
      plen     = std::min(plen, es_map_len);
      m_in->skip(plen);
      es_map_len -= 4 + plen;
    }

  } catch (...) {
  }

  m_in->setFilePointer(pos + len);
}

mpeg_ps_packet_c
mpeg_ps_reader_c::parse_packet(mpeg_ps_id_t id,
                               bool read_data) {
  mpeg_ps_packet_c packet{id};

  packet.m_id.sub_id   = 0;
  packet.m_length      = m_in->read_uint16_be();
  packet.m_full_length = packet.m_length;

  if (    (0xbc >  packet.m_id.id)
      || ((0xf0 <= packet.m_id.id) && (0xfd != packet.m_id.id))
      ||  (0xbf == packet.m_id.id)) {        // private 2 stream
    m_in->skip(packet.m_length);
    return packet;
  }

  if (0xbe == packet.m_id.id) {        // padding stream
    int64_t pos = m_in->getFilePointer();
    m_in->skip(packet.m_length);
    uint32_t header = m_in->read_uint32_be();
    if (mtx::mpeg::is_start_code(header))
      m_in->setFilePointer(pos + packet.m_length);

    else {
      mxdebug_if(m_debug_packets, fmt::format("mpeg_ps: [begin] padding stream length incorrect at {0}, find next header...\n", pos - 6));
      m_in->setFilePointer(pos);
      header = 0xffffffff;
      if (resync_stream(header)) {
        packet.m_full_length = m_in->getFilePointer() - pos - 4;
        mxdebug_if(m_debug_packets, fmt::format("mpeg_ps: [end] padding stream length adjusted from {0} to {1}\n", packet.m_length, packet.m_full_length));
        m_in->setFilePointer(pos + packet.m_full_length);
      }
    }

    return packet;
  }

  if (0 == packet.m_length)
    return packet;

  uint8_t c = 0;
  // Skip stuFFing bytes
  while (0 < packet.m_length) {
    c = m_in->read_uint8();
    packet.m_length--;
    if (c != 0xff)
      break;
  }

  // Skip STD buffer size
  if ((c & 0xc0) == 0x40) {
    if (2 > packet.m_length)
      return packet;
    packet.m_length -= 2;
    m_in->skip(1);
    c = m_in->read_uint8();
  }

  // Presentation time stamp
  if ((c & 0xf0) == 0x20) {
    if ((4 > packet.m_length) || !read_timestamp(c, packet.m_pts))
      return packet;
    packet.m_length -= 4;

  } else if ((c & 0xf0) == 0x30) {
    if ((9 > packet.m_length) || !read_timestamp(c, packet.m_pts) || !read_timestamp(m_in->read_uint8(), packet.m_dts))
      return packet;
    packet.m_length -= 4 + 5;

  } else if ((c & 0xc0) == 0x80) {
    if ((c & 0x30) != 0x00)
      mxerror_fn(m_ti.m_fname, Y("Reading encrypted VOBs is not supported.\n"));

    if (2 > packet.m_length)
      return packet;

    unsigned int flags   = m_in->read_uint8();
    unsigned int hdrlen  = m_in->read_uint8();
    packet.m_length     -= 2;

    if (hdrlen > packet.m_length)
      return packet;

    packet.m_length -= hdrlen;

    auto af_header = memory_c::alloc(hdrlen);
    if (m_in->read(af_header->get_buffer(), hdrlen) != hdrlen)
      return packet;

    mtx::bits::reader_c bc(af_header->get_buffer(), hdrlen);

    try {
      // PTS
      if (0x80 == (flags & 0x80))
        read_timestamp(bc, packet.m_pts);

      // DTS
      if (0x40 == (flags & 0x40))
        read_timestamp(bc, packet.m_dts);

      // PES extension?
      if ((0xfd == packet.m_id.id) && (0x01 == (flags & 0x01))) {
        int pes_ext_flags = bc.get_bits(8);

        if (0x80 == (pes_ext_flags & 0x80)) {
          // PES private data
          bc.skip_bits(128);
        }

        if (0x40 == (pes_ext_flags & 0x40)) {
          // pack header field
          int pack_header_field_len = bc.get_bits(8);
          bc.skip_bits(8 * pack_header_field_len);
        }

        if (0x20 == (pes_ext_flags & 0x20)) {
          // program packet sequence counter
          bc.skip_bits(16);
        }

        if (0x10 == (pes_ext_flags & 0x10)) {
          // P-STD buffer
          bc.skip_bits(16);
        }

        if (0x01 == (pes_ext_flags & 0x01)) {
          bc.skip_bits(1);
          int pes_ext2_len = bc.get_bits(7);

          if (0 < pes_ext2_len)
            packet.m_id.sub_id = bc.get_bits(8);
        }
      }
    } catch (...) {
    }

    if (0xbd == packet.m_id.id) {        // DVD audio substream
      if (4 > packet.m_length)
        return packet;
      packet.m_id.sub_id = m_in->read_uint8();
      packet.m_length--;

      if ((packet.m_id.sub_id & 0xe0) == 0x20)
        // Subtitles, not supported yet.
        return packet;

      else if (   ((0x80 <= packet.m_id.sub_id) && (0x8f >= packet.m_id.sub_id))
               || ((0x98 <= packet.m_id.sub_id) && (0xcf >= packet.m_id.sub_id))) {
        // Number of frames, startpos. MLP/TrueHD audio has a 4 byte header.
        auto audio_header_len = (0xb0 <= packet.m_id.sub_id) && (0xbf >= packet.m_id.sub_id) ? 4u : 3u;
        if (audio_header_len > packet.m_length)
          return packet;

        m_in->skip(audio_header_len);
        packet.m_length -= audio_header_len;
      }
    }

  } else if (0x0f != c)
    return packet;

  if (!read_data)
    packet.m_valid = true;

  else if (0 != packet.m_length)
    try {
      packet.m_buffer = memory_c::alloc(packet.m_length);
      packet.m_valid  = m_in->read(packet.m_buffer, packet.m_length) == packet.m_length;

    } catch (mtx::mm_io::exception &ex) {
      packet.m_valid  = false;
    }

  return packet;
}

void
mpeg_ps_reader_c::new_stream_v_avc_or_mpeg_1_2(mpeg_ps_id_t id,
                                               uint8_t *buf,
                                               unsigned int length,
                                               mpeg_ps_track_ptr &track) {
  try {
    m_in->save_pos();

    mtx::bytes::buffer_c buffer;
    buffer.add(buf, length);

    bool mpeg_12_seqhdr_found  = false;
    bool mpeg_12_picture_found = false;

    bool avc_seq_param_found   = false;
    bool avc_pic_param_found   = false;
    bool avc_slice_found       = false;
    bool avc_access_unit_found = false;

    uint64_t marker            = 0;
    std::size_t pos            = 0;

    while (4 > buffer.get_size()) {
      if (!find_next_packet_for_id(id, m_probe_range))
        throw false;

      auto packet = parse_packet(id);
      if (!packet)
        continue;

      buffer.add(packet.m_buffer->get_buffer(), packet.m_length);
    }

    marker = get_uint32_be(buffer.get_buffer());
    if (mtx::xyzvc::NALU_START_CODE == marker) {
      m_in->restore_pos();
      new_stream_v_avc(id, buf, length, track);
      return;
    }

    while (1) {
      auto ptr         = buffer.get_buffer();
      auto buffer_size = buffer.get_size();

      while (buffer_size > pos) {
        marker <<= 8;
        marker  |= ptr[pos];
        ++pos;

        if (((marker >> 8) & 0xffffffff) == 0x00000001) {
          // AVC
          int type = marker & 0x1f;

          switch (type) {
            case mtx::avc::NALU_TYPE_SEQ_PARAM:
              avc_seq_param_found   = true;
              break;

            case mtx::avc::NALU_TYPE_PIC_PARAM:
              avc_pic_param_found   = true;
              break;

            case mtx::avc::NALU_TYPE_NON_IDR_SLICE:
            case mtx::avc::NALU_TYPE_DP_A_SLICE:
            case mtx::avc::NALU_TYPE_DP_B_SLICE:
            case mtx::avc::NALU_TYPE_DP_C_SLICE:
            case mtx::avc::NALU_TYPE_IDR_SLICE:
              avc_slice_found       = true;
              break;

            case mtx::avc::NALU_TYPE_ACCESS_UNIT:
              avc_access_unit_found = true;
              break;
          }
        }

        if (mtx::mpeg::is_start_code(marker)) {
          // MPEG-1 or -2
          switch (marker & 0xffffffff) {
            case mtx::mpeg1_2::SEQUENCE_HEADER_START_CODE:
              mpeg_12_seqhdr_found  = true;
              break;

            case mtx::mpeg1_2::PICTURE_START_CODE:
              mpeg_12_picture_found = true;
              break;
          }

          if (mpeg_12_seqhdr_found && mpeg_12_picture_found) {
            m_in->restore_pos();
            new_stream_v_mpeg_1_2(id, buf, length, track);
            return;
          }
        }
      }

      if (!find_next_packet_for_id(id, m_probe_range))
        break;

      auto packet = parse_packet(id);
      if (!packet)
        continue;

      buffer.add(packet.m_buffer->get_buffer(), packet.m_length);
    }

    if (avc_seq_param_found && avc_pic_param_found && (avc_access_unit_found || avc_slice_found)) {
      m_in->restore_pos();
      new_stream_v_avc(id, buf, length, track);
      return;
    }

  } catch (...) {
  }

  m_in->restore_pos();
  throw false;
}

void
mpeg_ps_reader_c::new_stream_v_mpeg_1_2(mpeg_ps_id_t id,
                                        uint8_t *buf,
                                        unsigned int length,
                                        mpeg_ps_track_ptr &track) {
  std::shared_ptr<M2VParser> m2v_parser(new M2VParser);

  m2v_parser->SetProbeMode();
  m2v_parser->WriteData(buf, length);

  int num_leading_b_frames           = 0;
  MediaTime timestamp_b_frame_offset = 0;
  int state                          = m2v_parser->GetState();

  bool found_i_frame       = false;
  bool found_non_b_frame   = false;
  bool flushed             = false;
  MPEG2SequenceHeader seq_hdr;

  while (   (MPV_PARSER_STATE_EOS   != state)
         && (MPV_PARSER_STATE_ERROR != state)
         && (m_probe_range >= m_in->getFilePointer())) {
    if (find_next_packet_for_id(id, m_probe_range)) {
      auto packet = parse_packet(id);
      if (!packet)
        break;

      m2v_parser->WriteData(packet.m_buffer->get_buffer(), packet.m_length);

    } else if (flushed)
      break;

    else {
      m2v_parser->SetEOS();
      flushed = true;
    }

    state = m2v_parser->GetState();

    while (true) {
      std::shared_ptr<MPEGFrame> frame(m2v_parser->ReadFrame());
      if (!frame)
        break;

      if (!found_i_frame) {
        if ('I' != frame->frameType)
          continue;

        found_i_frame = true;
        seq_hdr       = m2v_parser->GetSequenceHeader();

        continue;
      }

      if ('B' != frame->frameType) {
        found_non_b_frame = true;
        break;
      }

      num_leading_b_frames++;
      timestamp_b_frame_offset += frame->duration;
    }

    if (found_i_frame && found_non_b_frame)
      break;
  }

  if ((MPV_PARSER_STATE_FRAME != state) || !found_i_frame || !m2v_parser->GetMPEGVersion() || !seq_hdr.width || !seq_hdr.height) {
    mxdebug_if(m_debug_headers, fmt::format("MPEG PS: blocking id 0x{0:02x}({1:02x}) for supposed type MPEG1/2\n", id.id, id.sub_id));
    m_blocked_ids[id.idx()] = true;
    throw false;
  }

  track->codec                    = codec_c::look_up(codec_c::type_e::V_MPEG12);
  track->v_interlaced             = !seq_hdr.progressiveSequence;
  track->v_version                = m2v_parser->GetMPEGVersion();
  track->v_width                  = seq_hdr.width;
  track->v_height                 = seq_hdr.height;
  track->v_field_duration         = mtx::rational(1'000'000'000, seq_hdr.frameRate * 2);
  track->v_aspect_ratio           = seq_hdr.aspectRatio;
  track->timestamp_b_frame_offset = mtx::to_int(timestamp_b_frame_offset);

  mxdebug_if(m_debug_timestamps,
             fmt::format("Leading B frames {0} rate {1} progressive? {2} offset {3} found_i? {4} found_non_b? {5}\n",
                         num_leading_b_frames, seq_hdr.frameRate, !!seq_hdr.progressiveSequence, track->timestamp_b_frame_offset, found_i_frame, found_non_b_frame));

  if ((0 >= track->v_aspect_ratio) || (1 == track->v_aspect_ratio))
    track->v_dwidth = track->v_width;
  else
    track->v_dwidth = mtx::to_int_rounded(track->v_height * track->v_aspect_ratio);
  track->v_dheight  = track->v_height;

  track->use_buffer(128000);
}

void
mpeg_ps_reader_c::new_stream_v_avc(mpeg_ps_id_t id,
                                   uint8_t *buf,
                                   unsigned int length,
                                   mpeg_ps_track_ptr &track) {
  mtx::avc::es_parser_c parser;

  parser.add_bytes(buf, length);

  while (!parser.headers_parsed() && (m_probe_range >= m_in->getFilePointer())) {
    if (!find_next_packet_for_id(id, m_probe_range))
      break;

    auto packet = parse_packet(id);
    if (!packet)
      break;

    parser.add_bytes(packet.m_buffer->get_buffer(), packet.m_length);
  }

  if (!parser.headers_parsed())
    throw false;

  track->codec    = codec_c::look_up(codec_c::type_e::V_MPEG4_P10);
  track->v_width  = parser.get_width();
  track->v_height = parser.get_height();

  if (parser.has_par_been_found()) {
    auto dimensions  = parser.get_display_dimensions();
    track->v_dwidth  = dimensions.first;
    track->v_dheight = dimensions.second;
  }
}

void
mpeg_ps_reader_c::new_stream_v_vc1(mpeg_ps_id_t id,
                                   uint8_t *buf,
                                   unsigned int length,
                                   mpeg_ps_track_ptr &track) {
  mtx::vc1::es_parser_c parser;

  parser.add_bytes(buf, length);

  while (!parser.is_sequence_header_available() && (m_probe_range >= m_in->getFilePointer())) {
    if (!find_next_packet_for_id(id, m_probe_range))
      break;

    auto packet = parse_packet(id);
    if (!packet)
      break;

    parser.add_bytes(packet.m_buffer->get_buffer(), packet.m_length);
  }

  if (!parser.is_sequence_header_available())
    throw false;

  mtx::vc1::sequence_header_t seqhdr;
  parser.get_sequence_header(seqhdr);

  track->codec              = codec_c::look_up(codec_c::type_e::V_VC1);
  track->v_width            = seqhdr.pixel_width;
  track->v_height           = seqhdr.pixel_height;
  track->provide_timestamps = true;

  track->use_buffer(512000);
}

void
mpeg_ps_reader_c::new_stream_a_mpeg(mpeg_ps_id_t,
                                    uint8_t *buf,
                                    unsigned int length,
                                    mpeg_ps_track_ptr &track) {
  mp3_header_t header;

  if (-1 == find_consecutive_mp3_headers(buf, length, 1, &header))
    throw false;

  track->a_channels    = header.channels;
  track->a_sample_rate = header.sampling_frequency;
  track->codec         = header.get_codec();
}

void
mpeg_ps_reader_c::new_stream_a_ac3(mpeg_ps_id_t id,
                                   uint8_t *buf,
                                   unsigned int length,
                                   mpeg_ps_track_ptr &track) {
  mxdebug_if(m_debug_headers, fmt::format("new_stream_a_ac3 for ID {0} buf len {1}\n", id, length));

  mtx::bytes::buffer_c buffer;

  buffer.add(buf, length);

  while (m_probe_range >= m_in->getFilePointer()) {
    mtx::ac3::frame_c header;

    if (-1 != header.find_in(buffer.get_buffer(), buffer.get_size())) {
      mxdebug_if(m_debug_headers,
                 fmt::format("new_stream_a_ac3 first AC-3 header bsid {0} channels {1} sample_rate {2} bytes {3} samples {4}\n",
                             header.m_bs_id, header.m_channels, header.m_sample_rate, header.m_bytes, header.m_samples));

      track->a_channels    = header.m_channels;
      track->a_sample_rate = header.m_sample_rate;
      track->a_bsid        = header.m_bs_id;
      track->codec         = header.get_codec();

      return;
    }

    if (!find_next_packet_for_id(id, m_probe_range))
      throw false;

    auto packet = parse_packet(id);
    if (!packet || (id.sub_id != packet.m_id.sub_id))
      continue;

    buffer.add(packet.m_buffer->get_buffer(), packet.m_length);
  }

  throw false;
}

void
mpeg_ps_reader_c::new_stream_a_dts(mpeg_ps_id_t id,
                                   uint8_t *buf,
                                   unsigned int length,
                                   mpeg_ps_track_ptr &track) {
  mtx::bytes::buffer_c buffer;

  buffer.add(buf, length);

  while ((-1 == mtx::dts::find_header(buffer.get_buffer(), buffer.get_size(), track->dts_header, false)) && (m_probe_range >= m_in->getFilePointer())) {
    if (!find_next_packet_for_id(id, m_probe_range))
      throw false;

    auto packet = parse_packet(id);
    if (!packet || (id.sub_id != packet.m_id.sub_id))
      continue;

    buffer.add(packet.m_buffer->get_buffer(), packet.m_length);
  }

  track->a_channels    = track->dts_header.get_total_num_audio_channels();
  track->a_sample_rate = track->dts_header.get_effective_sampling_frequency();

  track->codec.set_specialization(track->dts_header.get_codec_specialization());
}

void
mpeg_ps_reader_c::new_stream_a_truehd(mpeg_ps_id_t id,
                                      uint8_t *buf,
                                      unsigned int length,
                                      mpeg_ps_track_ptr &track) {
  mtx::truehd::parser_c parser;

  parser.add_data(buf, length);

  while (1) {
    while (parser.frame_available()) {
      mtx::truehd::frame_cptr frame = parser.get_next_frame();
      if (mtx::truehd::frame_t::sync != frame->m_type)
        continue;

      mxdebug_if(m_debug_headers,
                 fmt::format("first TrueHD header channels {0} sampling_rate {1} samples_per_frame {2}\n",
                             frame->m_channels, frame->m_sampling_rate, frame->m_samples_per_frame));

      track->codec         = frame->codec();
      track->a_channels    = frame->m_channels;
      track->a_sample_rate = frame->m_sampling_rate;

      return;
    }

    if (m_probe_range < m_in->getFilePointer())
      throw false;

    if (!find_next_packet_for_id(id, m_probe_range))
      throw false;

    auto packet = parse_packet(id);
    if (!packet || (id.sub_id != packet.m_id.sub_id))
      continue;

    parser.add_data(packet.m_buffer->get_buffer(), packet.m_length);
  }
}

void
mpeg_ps_reader_c::new_stream_a_pcm(mpeg_ps_id_t,
                                   uint8_t *buffer,
                                   unsigned int length,
                                   mpeg_ps_track_ptr &track) {
  static int const s_lpcm_frequency_table[4] = { 48000, 96000, 44100, 32000 };

  try {
    auto bc = mtx::bits::reader_c{buffer, length};
    bc.skip_bits(8);            // emphasis (1), muse(1), reserved(1), frame number(5)
    track->a_bits_per_sample = 16 + bc.get_bits(2) * 4;
    track->a_sample_rate     = s_lpcm_frequency_table[ bc.get_bits(2) ];
    bc.skip_bit();              // reserved
    track->a_channels        = bc.get_bits(3) + 1;
    bc.skip_bits(8);            // dynamic range control(8)

  } catch (...) {
    throw false;
  }

  if (28 == track->a_bits_per_sample)
    throw false;

  track->skip_packet_data_bytes = 3;
}

/*
  MPEG PS ids and their meaning:

  0xbd         audio substream; type determined by audio id in packet
  . 0x20..0x3f VobSub subtitles
  . 0x80..0x87 (E-)AC-3
  . 0x88..0x8f DTS
  . 0x98..0x9f DTS
  . 0xa0..0xaf PCM
  . 0xb0..0xbf TrueHD
  . 0xc0..0xc7 (E-)AC-3
  0xc0..0xdf   MP2 audio
  0xe0..0xef   MPEG-1/-2 video
  0xfd         VC-1 video

 */

void
mpeg_ps_reader_c::found_new_stream(mpeg_ps_id_t id) {
  mxdebug_if(m_debug_headers, fmt::format("MPEG PS: new stream id 0x{0:02x}\n", id.id));

  if (((0xc0 > id.id) || (0xef < id.id)) && (0xbd != id.id) && (0xfd != id.id))
    return;

  try {
    auto packet = parse_packet(id);
    if (!packet)
      throw false;

    id = packet.m_id;

    if (0xbd == id.id) {        // DVD audio substream
      mxdebug_if(m_debug_headers, fmt::format("MPEG PS:   audio substream id 0x{0:02x}\n", id.sub_id));
      if (0 == id.sub_id)
        return;
    }

    mxdebug_if(m_debug_timestamps && packet.has_pts(),
               fmt::format("Timestamp for track {0}: {1} [{2}] (DTS: {3})\n",
                           id, mtx::string::format_timestamp(packet.pts()), packet.pts() * 90 / 1000000ll,
                           packet.has_dts() ? fmt::format("{0} [{1}]", mtx::string::format_timestamp(packet.dts()), packet.dts() * 90 / 1000000ll) : "none"s));

    if (mtx::includes(m_blocked_ids, id.idx()))
      return;

    int64_t timestamp_for_offset = packet.pts();
    if (std::numeric_limits<int64_t>::max() == timestamp_for_offset)
      timestamp_for_offset = -1;

    if (mtx::includes(id2idx, id.idx())) {
      mpeg_ps_track_ptr &track = tracks[id2idx[id.idx()]];
      if ((-1 != timestamp_for_offset) && (-1 == track->timestamp_offset))
        track->timestamp_offset = timestamp_for_offset;
      return;
    }

    mpeg_ps_track_ptr track(new mpeg_ps_track_t);
    track->timestamp_offset = timestamp_for_offset;
    track->type            = '?';

    int es_type = es_map[id.id];
    if (0 != es_type) {
      switch (es_type) {
        case 0x01:
        case 0x02:
          track->type  = 'v';
          track->codec = codec_c::look_up(codec_c::type_e::V_MPEG12);
          break;
          break;
        case 0x03:
        case 0x04:
          track->type  = 'a';
          track->codec = codec_c::look_up(codec_c::type_e::A_MP3);
          break;
        case 0x0f:
        case 0x11:
          track->type  = 'a';
          track->codec = codec_c::look_up(codec_c::type_e::A_AAC);
          break;
        case 0x10:
          track->type  = 'v';
          track->codec = codec_c::look_up(codec_c::type_e::V_MPEG4_P2);
          break;
        case 0x1b:
          track->type  = 'v';
          track->codec = codec_c::look_up(codec_c::type_e::V_MPEG4_P10);
          break;
        case 0x80:
          track->type  = 'a';
          track->codec = codec_c::look_up(codec_c::type_e::A_PCM);
          break;
        case 0x81:
          track->type  = 'a';
          track->codec = codec_c::look_up(codec_c::type_e::A_AC3);
          break;
      }

    } else if (0xbd == id.id) {
      track->type = 'a';

      if ((0x20 <= id.sub_id) && (0x3f >= id.sub_id)) {
        track->type  = 's';
        track->codec = codec_c::look_up(codec_c::type_e::S_VOBSUB);

      } else if (((0x80 <= id.sub_id) && (0x87 >= id.sub_id)) || ((0xc0 <= id.sub_id) && (0xc7 >= id.sub_id)))
        track->codec = codec_c::look_up(codec_c::type_e::A_AC3);

      else if ((0x88 <= id.sub_id) && (0x9f >= id.sub_id))
        track->codec = codec_c::look_up(codec_c::type_e::A_DTS);

      else if ((0xa0 <= id.sub_id) && (0xa7 >= id.sub_id))
        track->codec = codec_c::look_up(codec_c::type_e::A_PCM);

      else if ((0xb0 <= id.sub_id) && (0xbf >= id.sub_id))
        track->codec = codec_c::look_up(codec_c::type_e::A_TRUEHD);

      else if ((0x80 <= id.sub_id) && (0x8f >= id.sub_id))
        track->codec = codec_c::look_up(codec_c::type_e::A_PCM);

      else
        track->type = '?';

    } else if ((0xc0 <= id.id) && (0xdf >= id.id)) {
      track->type  = 'a';
      track->codec = codec_c::look_up(codec_c::type_e::A_MP3);

    } else if ((0xe0 <= id.id) && (0xef >= id.id)) {
      track->type  = 'v';
      track->codec = codec_c::look_up(codec_c::type_e::V_MPEG12);

    } else if (0xfd == id.id) {
      track->type  = 'v';
      track->codec = codec_c::look_up(codec_c::type_e::V_VC1);
    }

    if ('?' == track->type)
      return;

    if (track->codec.is(codec_c::type_e::V_MPEG12))
      new_stream_v_avc_or_mpeg_1_2(id, packet.m_buffer->get_buffer(), packet.m_length, track);

    else if (track->codec.is(codec_c::type_e::A_MP3))
      new_stream_a_mpeg(id, packet.m_buffer->get_buffer(), packet.m_length, track);

    else if (track->codec.is(codec_c::type_e::A_AC3))
      new_stream_a_ac3(id, packet.m_buffer->get_buffer(), packet.m_length, track);

    else if (track->codec.is(codec_c::type_e::A_DTS))
      new_stream_a_dts(id, packet.m_buffer->get_buffer(), packet.m_length, track);

    else if (track->codec.is(codec_c::type_e::V_VC1))
      new_stream_v_vc1(id, packet.m_buffer->get_buffer(), packet.m_length, track);

    else if (track->codec.is(codec_c::type_e::A_TRUEHD))
      new_stream_a_truehd(id, packet.m_buffer->get_buffer(), packet.m_length, track);

    else if (track->codec.is(codec_c::type_e::A_PCM))
      new_stream_a_pcm(id, packet.m_buffer->get_buffer(), packet.m_length, track);

    else
      // Unsupported track type
      throw false;

    track->id        = id;
    id2idx[id.idx()] = tracks.size();
    tracks.push_back(track);

  } catch (bool) {
    m_blocked_ids[id.idx()] = true;

  } catch (...) {
    mxerror_fn(m_ti.m_fname, Y("Error parsing a MPEG PS packet during the header reading phase. This stream seems to be badly damaged.\n"));
  }
}

bool
mpeg_ps_reader_c::find_next_packet(mpeg_ps_id_t &id,
                                   int64_t max_file_pos) {
  try {
    uint32_t header;

    header = m_in->read_uint32_be();
    while (1) {
      uint8_t byte;

      if ((-1 != max_file_pos) && (m_in->getFilePointer() > static_cast<size_t>(max_file_pos)))
        return false;

      switch (header) {
        case mtx::mpeg1_2::PACKET_START_CODE:
          if (-1 == version) {
            byte = m_in->read_uint8();
            if ((byte & 0xc0) != 0)
              version = 2;      // MPEG-2 PS
            else
              version = 1;
            m_in->skip(-1);
          }

          m_in->skip(2 * 4);   // pack header
          if (2 == version) {
            m_in->skip(1);
            byte = m_in->read_uint8() & 0x07;
            m_in->skip(byte);  // stuffing bytes
          }
          header = m_in->read_uint32_be();
          break;

        case mtx::mpeg1_2::SYSTEM_HEADER_START_CODE:
          m_in->skip(2 * 4);   // system header
          byte = m_in->read_uint8();
          while ((byte & 0x80) == 0x80) {
            m_in->skip(2);     // P-STD info
            byte = m_in->read_uint8();
          }
          m_in->skip(-1);
          header = m_in->read_uint32_be();
          break;

        case mtx::mpeg1_2::MPEG_PROGRAM_END_CODE:
          if (!resync_stream(header))
            return false;
          break;

        case mtx::mpeg1_2::PROGRAM_STREAM_MAP_START_CODE:
          parse_program_stream_map();
          if (!resync_stream(header))
            return false;
          break;

        default:
          if (!mtx::mpeg::is_start_code(header)) {
            if (!resync_stream(header))
              return false;
            continue;
          }

          id.id = header & 0xff;
          return true;

          break;
      }
    }
  } catch(...) {
    return false;
  }
}

bool
mpeg_ps_reader_c::find_next_packet_for_id(mpeg_ps_id_t id,
                                          int64_t max_file_pos) {
  try {
    mpeg_ps_id_t new_id;
    while (find_next_packet(new_id, max_file_pos)) {
      if (id.id == new_id.id)
        return true;
      m_in->skip(m_in->read_uint16_be());
    }
  } catch(...) {
  }
  return false;
}

bool
mpeg_ps_reader_c::resync_stream(uint32_t &header) {
  mxdebug_if(m_debug_resync, fmt::format("MPEG PS: synchronisation lost at {0}; looking for start code\n", m_in->getFilePointer()));

  try {
    while (1) {
      header <<= 8;
      header  |= m_in->read_uint8();
      if (mtx::mpeg::is_start_code(header))
        break;
    }

    mxdebug_if(m_debug_resync, fmt::format("resync succeeded at {0}, header 0x{1:08x}\n", m_in->getFilePointer() - 4, header));

    return true;

  } catch (...) {
    mxdebug_if(m_debug_resync, "resync failed: exception caught\n");
    return false;
  }
}

void
mpeg_ps_reader_c::create_packetizer(int64_t id) {
  if ((0 > id) || (tracks.size() <= static_cast<size_t>(id)))
    return;
  if (0 == tracks[id]->ptzr)
    return;
  if (!demuxing_requested(tracks[id]->type, id))
    return;

  m_ti.m_private_data.reset();
  m_ti.m_id = id;

  auto &track = tracks[id];

  if ('a' == track->type) {
    if (track->codec.is(codec_c::type_e::A_MP3)) {
      track->ptzr = add_packetizer(new mp3_packetizer_c(this, m_ti, track->a_sample_rate, track->a_channels, true));
      show_packetizer_info(id, ptzr(track->ptzr));

    } else if (track->codec.is(codec_c::type_e::A_AC3)) {
      track->ptzr = add_packetizer(new ac3_packetizer_c(this, m_ti, track->a_sample_rate, track->a_channels, track->a_bsid));
      show_packetizer_info(id, ptzr(track->ptzr));

    } else if (track->codec.is(codec_c::type_e::A_DTS)) {
      track->ptzr = add_packetizer(new dts_packetizer_c(this, m_ti, track->dts_header));
      show_packetizer_info(id, ptzr(track->ptzr));

    } else if (track->codec.is(codec_c::type_e::A_TRUEHD)) {
      track->ptzr = add_packetizer(new truehd_packetizer_c(this, m_ti, mtx::truehd::frame_t::truehd, track->a_sample_rate, track->a_channels));
      show_packetizer_info(id, ptzr(track->ptzr));

    } else if (track->codec.is(codec_c::type_e::A_PCM)) {
      track->ptzr = add_packetizer(new pcm_packetizer_c(this, m_ti, track->a_sample_rate, track->a_channels, track->a_bits_per_sample, pcm_packetizer_c::big_endian_integer));
      show_packetizer_info(id, ptzr(track->ptzr));

    } else
      mxerror(fmt::format(FY("mpeg_ps_reader: Should not have happened #1. {0}"), BUGMSG));

  } else {                      // if (track->type == 'a')
    if (track->codec.is(codec_c::type_e::V_MPEG12)) {
      generic_packetizer_c *m2vpacketizer;

      m2vpacketizer       = new mpeg1_2_video_packetizer_c(this, m_ti, track->v_version, mtx::to_int(track->v_field_duration * 2), track->v_width, track->v_height,
                                                           track->v_dwidth, track->v_dheight, false);
      track->ptzr         = add_packetizer(m2vpacketizer);
      show_packetizer_info(id, ptzr(track->ptzr));
      m2vpacketizer->set_video_interlaced_flag(track->v_interlaced);

    } else if (track->codec.is(codec_c::type_e::V_MPEG4_P10)) {
      track->ptzr = add_packetizer(new avc_es_video_packetizer_c(this, m_ti, track->v_width, track->v_height));
      show_packetizer_info(id, ptzr(track->ptzr));

    } else if (track->codec.is(codec_c::type_e::V_VC1)) {
      track->ptzr = add_packetizer(new vc1_video_packetizer_c(this, m_ti));
      show_packetizer_info(id, ptzr(track->ptzr));

    } else
      mxerror(fmt::format(FY("mpeg_ps_reader: Should not have happened #2. {0}"), BUGMSG));
  }

  if (-1 != track->timestamp_offset)
    ptzr(track->ptzr).m_ti.m_tcsync.displacement += track->timestamp_offset;

  m_ptzr_to_track_map[ &ptzr(track->ptzr) ] = track;
}

void
mpeg_ps_reader_c::create_packetizers() {
  size_t i;

  for (i = 0; i < tracks.size(); i++)
    create_packetizer(i);
}

void
mpeg_ps_reader_c::add_available_track_ids() {
  add_available_track_id_range(tracks.size());
}

file_status_e
mpeg_ps_reader_c::read(generic_packetizer_c *requested_ptzr,
                       bool force) {
  if (file_done)
    return flush_packetizers();

  auto num_queued_bytes = get_queued_bytes();
  if (!force && (20 * 1024 * 1024 < num_queued_bytes)) {
    mpeg_ps_track_ptr requested_ptzr_track = m_ptzr_to_track_map[requested_ptzr];
    if (!requested_ptzr_track || (('a' != requested_ptzr_track->type) && ('v' != requested_ptzr_track->type)) || (64 * 1024 * 1024 < num_queued_bytes))
      return FILE_STATUS_HOLDING;
  }

  try {
    mpeg_ps_id_t new_id;
    while (find_next_packet(new_id)) {
      auto packet_pos = m_in->getFilePointer() - 4;
      auto packet     = parse_packet(new_id, false);
      new_id          = packet.m_id;

      if (!packet) {
        if (    (0xbe != new_id.id)       // padding stream
             && (0xbf != new_id.id))      // private 2 stream (navigation data)
          mxdebug_if(m_debug_packets, fmt::format("mpeg_ps: parse_packet failed at {0}, skipping {1}\n", packet_pos, packet.m_full_length));
        m_in->setFilePointer(packet_pos + 4 + 2 + packet.m_full_length);
        continue;
      }

      if (!mtx::includes(id2idx, new_id.idx()) || (-1 == tracks[id2idx[new_id.idx()]]->ptzr)) {
        m_in->setFilePointer(packet_pos + 4 + 2 + packet.m_full_length);
        continue;
      }

      mxdebug_if(m_debug_packets, fmt::format("mpeg_ps: packet at {0}: {1}\n", packet_pos, packet));

      auto track = tracks[id2idx[new_id.idx()]];

      int64_t timestamp = packet.has_pts() ? packet.pts() : -1;
      if ((-1 != timestamp) && track->provide_timestamps)
        timestamp = std::max<int64_t>(timestamp - global_timestamp_offset, -1);

      else
        timestamp = -1;

      if (track->skip_packet_data_bytes) {
        auto bytes_to_skip = std::min(packet.m_length, track->skip_packet_data_bytes);
        packet.m_length   -= bytes_to_skip;
        m_in->skip(bytes_to_skip);
      }

      if (0 < track->buffer_size) {
        if (((track->buffer_usage + packet.m_length) > track->buffer_size)) {
          auto new_packet = std::make_shared<packet_t>(memory_c::borrow(track->buffer, track->buffer_usage));

          if (!track->multiple_timestamps_packet_extension->empty()) {
            new_packet->extensions.push_back(packet_extension_cptr(track->multiple_timestamps_packet_extension));
            track->multiple_timestamps_packet_extension = new multiple_timestamps_packet_extension_c;
          }

          ptzr(track->ptzr).process(new_packet);
          track->buffer_usage = 0;
        }

        track->assert_buffer_size(packet.m_length);

        if (m_in->read(&track->buffer[track->buffer_usage], packet.m_length) != packet.m_length) {
          mxdebug_if(m_debug_packets, "mpeg_ps: file_done: m_in->read\n");
          return finish();
        }

        if (-1 != timestamp)
          track->multiple_timestamps_packet_extension->add(timestamp, track->buffer_usage);

        track->buffer_usage += packet.m_length;

      } else {
        auto buf = memory_c::alloc(packet.m_length);

        if (m_in->read(buf, packet.m_length) != packet.m_length) {
          mxdebug_if(m_debug_packets, "mpeg_ps: file_done: m_in->read\n");
          return finish();
        }

        ptzr(track->ptzr).process(std::make_shared<packet_t>(buf, timestamp));
      }

      return FILE_STATUS_MOREDATA;
    }
    mxdebug_if(m_debug_packets, "mpeg_ps: file_done: !find_next_packet\n");

  } catch(...) {
    mxdebug_if(m_debug_packets, "mpeg_ps: file_done: exception\n");
  }

  return finish();
}

file_status_e
mpeg_ps_reader_c::finish() {
  if (file_done)
    return flush_packetizers();

  for (auto &track : tracks)
    if (0 < track->buffer_usage)
      ptzr(track->ptzr).process(std::make_shared<packet_t>(memory_c::clone(track->buffer, track->buffer_usage)));

  file_done = true;

  return flush_packetizers();
}

void
mpeg_ps_reader_c::identify() {
  auto info = mtx::id::info_c{};

  auto multi_in = dynamic_cast<mm_multi_file_io_c *>(get_underlying_input());
  if (multi_in)
    multi_in->create_verbose_identification_info(info);

  id_result_container(info.get());

  size_t i;
  for (i = 0; i < tracks.size(); i++) {
    mpeg_ps_track_ptr &track = tracks[i];

    info = mtx::id::info_c{};

    if (track->codec.is(codec_c::type_e::V_MPEG4_P10))
      info.add(mtx::id::packetizer, mtx::id::mpeg4_p10_es_video);

    info.set(mtx::id::number,        (static_cast<uint64_t>(track->id.sub_id) << 32) | static_cast<uint64_t>(track->id.id));
    info.add(mtx::id::stream_id,     track->id.id);
    info.add(mtx::id::sub_stream_id, track->id.sub_id);
    info.add_joined(mtx::id::display_dimensions, "x"s, track->v_dwidth, track->v_dheight);

    if ('a' == track->type) {
      info.add(mtx::id::audio_channels,           track->a_channels);
      info.add(mtx::id::audio_sampling_frequency, track->a_sample_rate);
      info.add(mtx::id::audio_bits_per_sample,    track->a_bits_per_sample);

    } else if ('v' == track->type)
      info.add_joined(mtx::id::pixel_dimensions, "x"s, track->v_width, track->v_height);

    id_result_track(i, 'a' == track->type ? ID_RESULT_TRACK_AUDIO : ID_RESULT_TRACK_VIDEO, track->codec.get_name(), info.get());
  }
}
