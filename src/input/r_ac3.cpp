/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   AC-3 demultiplexer module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <algorithm>

#include "common/codec.h"
#include "common/id3.h"
#include "common/id_info.h"
#include "common/mm_io_x.h"
#include "input/r_ac3.h"
#include "merge/input_x.h"
#include "output/p_ac3.h"

bool
ac3_reader_c::probe_file() {
  mtx::id3::skip_v2_tag(*m_in);

  m_first_header_offset = find_valid_headers(*m_in, m_probe_range_info.probe_size, m_probe_range_info.num_headers);
  return ( m_probe_range_info.require_headers_at_start && (0 == m_first_header_offset))
      || (!m_probe_range_info.require_headers_at_start && (0 <= m_first_header_offset));
}

void
ac3_reader_c::read_headers() {
  try {
    int tag_size_start = mtx::id3::skip_v2_tag(*m_in);
    int tag_size_end   = mtx::id3::tag_present_at_end(*m_in);

    if (0 > tag_size_start)
      tag_size_start = 0;
    if (0 < tag_size_end)
      m_size -= tag_size_end;

    size_t init_read_len = std::min(m_size - tag_size_start, static_cast<uint64_t>(m_chunk->get_size()));

    if (m_in->read(m_chunk->get_buffer(), init_read_len) != init_read_len)
      throw mtx::input::header_parsing_x();

    m_in->setFilePointer(tag_size_start);

    mtx::ac3::parser_c parser;
    parser.add_bytes(m_chunk->get_buffer(), init_read_len);
    if (!parser.frame_available())
      throw mtx::input::header_parsing_x();
    m_ac3header = parser.get_frame();

  } catch (mtx::mm_io::exception &) {
    throw mtx::input::open_x();
  }

  show_demuxer_info();
}

void
ac3_reader_c::create_packetizer(int64_t) {
  if (!demuxing_requested('a', 0) || !m_reader_packetizers.empty())
    return;

  add_packetizer(new ac3_packetizer_c(this, m_ti, m_ac3header.m_sample_rate, m_ac3header.m_channels, m_ac3header.m_bs_id));
  show_packetizer_info(0, ptzr(0));
}

file_status_e
ac3_reader_c::read(generic_packetizer_c *,
                   bool) {
  uint64_t remaining_bytes = m_size - m_in->getFilePointer();
  uint64_t read_len        = std::min(static_cast<uint64_t>(m_chunk->get_size()), remaining_bytes);
  int num_read             = m_in->read(m_chunk->get_buffer(), read_len);

  if (0 < num_read)
    ptzr(0).process(std::make_shared<packet_t>(memory_c::borrow(m_chunk->get_buffer(), num_read)));

  return (0 != num_read) && (0 < (remaining_bytes - num_read)) ? FILE_STATUS_MOREDATA : flush_packetizers();
}

void
ac3_reader_c::identify() {
  auto info = mtx::id::info_c{};
  info.add(mtx::id::audio_channels,           m_ac3header.m_channels);
  info.add(mtx::id::audio_sampling_frequency, m_ac3header.m_sample_rate);

  id_result_container();
  id_result_track(0, ID_RESULT_TRACK_AUDIO, m_ac3header.get_codec().get_name("AC-3"), info.get());
}

int
ac3_reader_c::find_valid_headers(mm_io_c &in,
                                 int64_t probe_range,
                                 int num_headers) {
  try {
    memory_cptr buf(memory_c::alloc(probe_range));

    in.setFilePointer(0);
    mtx::id3::skip_v2_tag(in);

    mtx::ac3::parser_c parser;
    int num_read = in.read(buf->get_buffer(), probe_range);
    int pos      = parser.find_consecutive_frames(buf->get_buffer(), num_read, num_headers);

    in.setFilePointer(0);

    return pos;

  } catch (...) {
    return -1;
  }
}
