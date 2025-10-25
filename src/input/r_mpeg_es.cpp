/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   MPEG ES (elementary stream) demultiplexer module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <cstring>

#include "common/codec.h"
#include "common/debugging.h"
#include "common/endian.h"
#include "common/error.h"
#include "common/mm_io_x.h"
#include "common/mpeg.h"
#include "common/id_info.h"
#include "input/r_mpeg_es.h"
#include "merge/input_x.h"
#include "merge/file_status.h"
#include "mpegparser/M2VParser.h"
#include "output/p_mpeg1_2.h"

namespace {
debugging_option_c s_debug{"mpeg_es_reader"};

constexpr auto READ_SIZE = 1024 * 1024;
}

bool
mpeg_es_reader_c::probe_file() {
  auto debug    = debugging_option_c{"mpeg_es_detection|mpeg_es_probe"};

  auto af_buf   = memory_c::alloc(READ_SIZE);
  auto buf      = af_buf->get_buffer();
  auto num_read = m_in->read(buf, READ_SIZE);

  if (4 > num_read)
    return 0;

  m_in->setFilePointer(0);

  // MPEG TS starts with 0x47.
  if (0x47 == buf[0])
    return 0;

  // MPEG PS starts with 0x000001ba.
  uint32_t value = get_uint32_be(buf);
  if (mtx::mpeg1_2::PACKET_START_CODE == value)
    return 0;

  auto num_slice_start_codes_found = 0u;
  auto start_code_at_beginning     = mtx::mpeg::is_start_code(value);
  auto gop_start_code_found        = false;
  auto sequence_start_code_found   = false;
  auto ext_start_code_found        = false;
  auto picture_start_code_found    = false;

  auto ok                          = false;

  // Let's look for a MPEG ES start code inside the first 1 MB.
  std::size_t i = 4;
  for (; i < num_read - 1; i++) {
    if (mtx::mpeg::is_start_code(value)) {
      mxdebug_if(debug, fmt::format("mpeg_es_detection: start code found; fourth byte: 0x{0:02x}\n", value & 0xff));

      if (mtx::mpeg1_2::SEQUENCE_HEADER_START_CODE == value)
        sequence_start_code_found = true;

      else if (mtx::mpeg1_2::PICTURE_START_CODE == value)
        picture_start_code_found = true;

      else if (mtx::mpeg1_2::GROUP_OF_PICTURES_START_CODE == value)
        gop_start_code_found = true;

      else if (mtx::mpeg1_2::EXT_START_CODE == value)
        ext_start_code_found = true;

      else if ((mtx::mpeg1_2::SLICE_START_CODE_LOWER <= value) && (mtx::mpeg1_2::SLICE_START_CODE_UPPER >= value))
        ++num_slice_start_codes_found;

      ok = sequence_start_code_found
        && picture_start_code_found
        && (   ((num_slice_start_codes_found > 0) && start_code_at_beginning)
            || ((num_slice_start_codes_found > 0) && gop_start_code_found && ext_start_code_found)
            || (num_slice_start_codes_found >= 25));

      if (ok)
        break;
    }

    value <<= 8;
    value  |= buf[i];
  }

  mxdebug_if(debug,
             fmt::format("mpeg_es_detection: sequence {0} picture {1} gop {2} ext {3} #slice {4} start code at beginning {5}; examined {6} bytes\n",
                         sequence_start_code_found, picture_start_code_found, gop_start_code_found, ext_start_code_found, num_slice_start_codes_found, start_code_at_beginning, i));

  if (!ok)
    return 0;

  // Let's try to read one frame.
  M2VParser parser;
  parser.SetProbeMode();

  return read_frame(parser, *m_in, READ_SIZE);
}

void
mpeg_es_reader_c::read_headers() {
  try {
    M2VParser parser;

    // Let's find the first frame. We need its information like
    // resolution, MPEG version etc.
    parser.SetProbeMode();
    if (!read_frame(parser, *m_in, 1024 * 1024))
      throw mtx::input::header_parsing_x();

    m_in->setFilePointer(0);

    MPEG2SequenceHeader seq_hdr = parser.GetSequenceHeader();
    version                     = parser.GetMPEGVersion();
    interlaced                  = !seq_hdr.progressiveSequence;
    width                       = seq_hdr.width;
    height                      = seq_hdr.height;
    field_duration              = mtx::rational(1'000'000'000, seq_hdr.frameRate * 2);
    aspect_ratio                = seq_hdr.aspectRatio;

    if ((0 >= aspect_ratio) || (1 == aspect_ratio))
      dwidth = width;
    else
      dwidth = mtx::to_int_rounded(height * aspect_ratio);
    dheight = height;

    mxdebug_if(s_debug, fmt::format("mpeg_es_reader: version {0} width {1} height {2} FPS {3} AR {4}\n", version, width, height, seq_hdr.frameRate, aspect_ratio));

  } catch (mtx::mm_io::exception &) {
    throw mtx::input::open_x();
  }
  show_demuxer_info();
}

void
mpeg_es_reader_c::create_packetizer(int64_t) {
  generic_packetizer_c *m2vpacketizer;
  if (!demuxing_requested('v', 0) || !m_reader_packetizers.empty())
    return;

  m2vpacketizer = new mpeg1_2_video_packetizer_c(this, m_ti, version, mtx::to_int(field_duration * 2), width, height, dwidth, dheight, false);
  add_packetizer(m2vpacketizer);
  m2vpacketizer->set_video_interlaced_flag(interlaced);

  show_packetizer_info(0, *m2vpacketizer);
}

file_status_e
mpeg_es_reader_c::read(generic_packetizer_c *,
                       bool) {
  int64_t bytes_to_read = std::min(static_cast<uint64_t>(READ_SIZE), m_size - m_in->getFilePointer());
  if (0 >= bytes_to_read)
    return flush_packetizers();

  memory_cptr chunk = memory_c::alloc(bytes_to_read);
  int64_t num_read  = m_in->read(chunk, bytes_to_read);

  if (0 < num_read) {
    chunk->set_size(num_read);
    ptzr(0).process(std::make_shared<packet_t>(chunk));
  }

  return bytes_to_read > num_read ? flush_packetizers() : FILE_STATUS_MOREDATA;
}

bool
mpeg_es_reader_c::read_frame(M2VParser &parser,
                             mm_io_c &in,
                             int64_t max_size) {
  auto af_buffer = memory_c::alloc(READ_SIZE);
  auto buffer    = af_buffer->get_buffer();
  int bytes_probed = 0;

  while (true) {
    auto state = parser.GetState();

    if (MPV_PARSER_STATE_FRAME == state)
      return true;

    if ((MPV_PARSER_STATE_EOS == state) || (MPV_PARSER_STATE_ERROR == state))
      return false;

    assert(MPV_PARSER_STATE_NEED_DATA == state);

    if ((max_size != -1) && (bytes_probed > max_size))
      return false;

    int bytes_read = in.read(buffer, std::min<int>(parser.GetFreeBufferSpace(), READ_SIZE));
    if (!bytes_read)
      return false;

    bytes_probed += bytes_read;

    parser.WriteData(buffer, bytes_read);
    parser.SetEOS();
  }
}

void
mpeg_es_reader_c::identify() {
  auto codec = fmt::format("mpg{0}", version);
  auto info  = mtx::id::info_c{};
  info.add_joined(mtx::id::pixel_dimensions, "x"s, width, height);

  id_result_container();
  id_result_track(0, ID_RESULT_TRACK_VIDEO, codec_c::get_name(codec, codec), info.get());
}
