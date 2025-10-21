/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   MP3 reader module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/codec.h"
#include "common/error.h"
#include "common/hacks.h"
#include "common/id3.h"
#include "common/id_info.h"
#include "common/mm_io_x.h"
#include "input/r_mp3.h"
#include "merge/input_x.h"
#include "output/p_mp3.h"

bool
mp3_reader_c::probe_file() {
  mtx::id3::skip_v2_tag(*m_in);

  m_first_header_offset = find_valid_headers(*m_in, m_probe_range_info.probe_size, m_probe_range_info.num_headers);
  return ( m_probe_range_info.require_headers_at_start && (0 == m_first_header_offset))
      || (!m_probe_range_info.require_headers_at_start && (0 <= m_first_header_offset));
}

void
mp3_reader_c::read_headers() {
  try {
    auto tag_size_start = mtx::id3::skip_v2_tag(*m_in);
    auto tag_size_end   = mtx::id3::tag_present_at_end(*m_in);

    if (0 > tag_size_start)
      tag_size_start = 0;
    if (0 < tag_size_end)
      m_size -= tag_size_end;

    auto init_read_len = std::min(m_size - tag_size_start, static_cast<uint64_t>(m_chunk->get_size()));

    int pos = find_valid_headers(*m_in, init_read_len, 5);
    if (0 > pos)
      throw mtx::input::header_parsing_x();

    m_in->setFilePointer(tag_size_start + pos);
    m_in->read(m_chunk->get_buffer(), 4);

    decode_mp3_header(m_chunk->get_buffer(), &m_mp3header);

    m_in->setFilePointer(tag_size_start + pos);

    show_demuxer_info();

    if ((0 < pos) && verbose)
      mxwarn_fn(m_ti.m_fname, fmt::format(FY("Skipping {0} bytes at the beginning (no valid MP3 header found).\n"), pos));

  } catch (mtx::mm_io::exception &) {
    throw mtx::input::open_x();
  }
}

void
mp3_reader_c::create_packetizer(int64_t) {
  if (!demuxing_requested('a', 0) || !m_reader_packetizers.empty())
    return;

  add_packetizer(new mp3_packetizer_c(this, m_ti, m_mp3header.sampling_frequency, m_mp3header.channels, false));
  show_packetizer_info(0, ptzr(0));
}

file_status_e
mp3_reader_c::read(generic_packetizer_c *,
                   bool) {
  int nread = m_in->read(m_chunk->get_buffer(), m_chunk->get_size());
  if (0 >= nread)
    return flush_packetizers();

  ptzr(0).process(std::make_shared<packet_t>(memory_c::borrow(m_chunk->get_buffer(), nread)));

  return FILE_STATUS_MOREDATA;
}

void
mp3_reader_c::identify() {
  auto info = mtx::id::info_c{};
  info.add(mtx::id::audio_channels,           m_mp3header.channels);
  info.add(mtx::id::audio_sampling_frequency, m_mp3header.sampling_frequency);

  id_result_container();
  id_result_track(0, ID_RESULT_TRACK_AUDIO, m_mp3header.get_codec().get_name(), info.get());
}

int
mp3_reader_c::find_valid_headers(mm_io_c &io,
                                 int64_t probe_range,
                                 int num_headers) {
  try {
    io.setFilePointer(0);
    mtx::id3::skip_v2_tag(io);

    memory_cptr buf = memory_c::alloc(probe_range);
    int nread       = io.read(buf->get_buffer(), probe_range);

    // auto header = mp3_header_t{};
    // auto idx    = find_mp3_header(buf->get_buffer(), std::min(nread, 32));

    // if ((0 == idx) && decode_mp3_header(&buf->get_buffer()[idx], &header) && header.is_tag) {
    //   probe_range += header.framesize;
    //   buf->resize(probe_range);

    //   io.setFilePointer(0);
    //   nread = io.read(buf->get_buffer(), probe_range);
    // }

    io.setFilePointer(0);

    return find_consecutive_mp3_headers(buf->get_buffer(), nread, num_headers);
    // auto result = find_consecutive_mp3_headers(buf->get_buffer(), nread, num_headers);
    // return -1 == result ? -1 : result + idx;

  } catch (...) {
    return -1;
  }
}
