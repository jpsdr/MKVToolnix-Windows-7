/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   TrueHD demultiplexer module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/codec.h"
#include "common/error.h"
#include "common/id3.h"
#include "common/id_info.h"
#include "common/mm_io_x.h"
#include "input/r_truehd.h"
#include "merge/input_x.h"
#include "output/p_ac3.h"
#include "output/p_truehd.h"

bool
truehd_reader_c::probe_file() {
  mtx::id3::skip_v2_tag(*m_in);
  m_chunk = memory_c::alloc(m_probe_range_info.probe_size);
  return find_valid_headers(*m_in, m_chunk->get_size(), 2);
}

void
truehd_reader_c::read_headers() {
  try {
    m_chunk            = memory_c::alloc(m_probe_range_info.probe_size);

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

    mtx::truehd::parser_c parser;
    parser.add_data(m_chunk->get_buffer(), init_read_len);


    auto found_truehd = false;
    auto found_ac3    = false;

    while (parser.frame_available() && (!found_truehd || !found_ac3)) {
      auto frame = parser.get_next_frame();

      if (frame->is_ac3()) {
        if (!found_ac3) {
          found_ac3    = true;
          m_ac3_header = frame->m_ac3_header;
        }

        continue;
      }

      if (!frame->is_sync())
        continue;

      m_header     = frame;
      found_truehd = true;
    }

    show_demuxer_info();

  } catch (mtx::mm_io::exception &) {
    throw mtx::input::open_x();
  }
}

void
truehd_reader_c::add_available_track_ids() {
  add_available_track_id(0);
  if (m_ac3_header.m_valid)
    add_available_track_id(1);
}

void
truehd_reader_c::create_packetizers() {
  create_packetizer(0);
  create_packetizer(1);
}

void
truehd_reader_c::create_packetizer(int64_t tid) {
  if (!demuxing_requested('a', tid))
    return;

  m_ti.m_id = tid;

  if (0 == tid) {
    m_truehd_ptzr = add_packetizer(new truehd_packetizer_c(this, m_ti, m_header->m_codec, m_header->m_sampling_rate, m_header->m_channels));
    show_packetizer_info(0, ptzr(m_truehd_ptzr));
    m_converter.set_packetizer(&ptzr(m_truehd_ptzr));

  } else if ((1 == tid) && m_ac3_header.m_valid) {
    m_ac3_ptzr = add_packetizer(new ac3_packetizer_c(this, m_ti, m_ac3_header.m_sample_rate, m_ac3_header.m_channels, m_ac3_header.m_bs_id));
    show_packetizer_info(m_ti.m_id, ptzr(m_ac3_ptzr));
    m_converter.set_ac3_packetizer(&ptzr(m_ac3_ptzr));

  }
}

file_status_e
truehd_reader_c::read(generic_packetizer_c *,
                      bool) {
  auto remaining_bytes = m_size - m_in->getFilePointer();
  auto read_len        = std::min<uint64_t>(m_chunk->get_size(), remaining_bytes);

  if (0 >= read_len) {
    m_converter.flush();
    return FILE_STATUS_DONE;
  }

  auto num_read = m_in->read(m_chunk->get_buffer(), read_len);

  if (0 < num_read)
    m_converter.convert(std::make_shared<packet_t>(memory_c::borrow(m_chunk->get_buffer(), num_read)));

  if (num_read == read_len)
    return FILE_STATUS_MOREDATA;

  m_converter.flush();
  return FILE_STATUS_DONE;
}

void
truehd_reader_c::identify() {
  id_result_container();

  auto info = mtx::id::info_c{};
  info.add(mtx::id::audio_channels,           m_header->m_channels);
  info.add(mtx::id::audio_sampling_frequency, m_header->m_sampling_rate);
  if (m_ac3_header.m_valid)
    info.set(mtx::id::multiplexed_tracks, std::vector<uint64_t>{{0, 1}});

  id_result_track(0, ID_RESULT_TRACK_AUDIO, m_header->codec().get_name(), info.get());

  if (!m_ac3_header.m_valid)
    return;

  info = mtx::id::info_c{};
  info.add(mtx::id::audio_channels,           m_ac3_header.m_channels);
  info.add(mtx::id::audio_sampling_frequency, m_ac3_header.m_sample_rate);
  info.set(mtx::id::multiplexed_tracks,       std::vector<uint64_t>{{0, 1}});

  id_result_track(1, ID_RESULT_TRACK_AUDIO, m_ac3_header.get_codec().get_name(), info.get());
}

bool
truehd_reader_c::find_valid_headers(mm_io_c &in,
                                    int64_t probe_range,
                                    int num_headers) {
  try {
    memory_cptr buf(memory_c::alloc(probe_range));

    in.setFilePointer(0);
    mtx::id3::skip_v2_tag(in);

    int num_read = in.read(buf->get_buffer(), probe_range);

    mtx::truehd::parser_c parser;
    parser.add_data(buf->get_buffer(), num_read);

    int num_sync_frames = 0;
    while (parser.frame_available()) {
      auto frame = parser.get_next_frame();
      if (!frame)
        break;

      if (frame->is_ac3())
        continue;

      if (frame->is_sync())
        ++num_sync_frames;
    }

    return num_sync_frames >= num_headers;

  } catch (...) {
    return false;
  }
}
