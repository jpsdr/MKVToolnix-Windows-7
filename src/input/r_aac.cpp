/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   AAC demultiplexer module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <algorithm>

#include "common/codec.h"
#include "common/id3.h"
#include "common/id_info.h"
#include "input/r_aac.h"
#include "merge/file_status.h"
#include "merge/input_x.h"
#include "merge/output_control.h"
#include "output/p_aac.h"

bool
aac_reader_c::probe_file() {
  m_first_header_offset = find_valid_headers(*m_in, m_probe_range_info.probe_size, m_probe_range_info.num_headers);

  return ( m_probe_range_info.require_headers_at_start && (0 == m_first_header_offset))
      || (!m_probe_range_info.require_headers_at_start && (0 <= m_first_header_offset));
}

void
aac_reader_c::read_headers() {
  try {
    int tag_size_start = mtx::id3::skip_v2_tag(*m_in);
    int tag_size_end   = mtx::id3::tag_present_at_end(*m_in);

    if (0 > tag_size_start)
      tag_size_start = 0;
    if (0 < tag_size_end)
      m_size -= tag_size_end;

    size_t init_read_len = std::min(m_size - tag_size_start - std::max(m_first_header_offset, 0), static_cast<uint64_t>(m_chunk->get_size()));

    if (m_first_header_offset >= 0)
      m_in->setFilePointer(m_first_header_offset);

    if (m_in->read(m_chunk, init_read_len) != init_read_len)
      throw mtx::input::header_parsing_x();

    m_in->setFilePointer(tag_size_start);

    m_parser.copy_data(false);
    m_parser.add_bytes(m_chunk->get_buffer(), init_read_len);

    if (!m_parser.headers_parsed())
      throw mtx::input::header_parsing_x();

    while (m_parser.frames_available()) {
      m_aacheader = m_parser.get_frame().m_header;
      if ((m_aacheader.config.sample_rate > 0) && (m_aacheader.config.channels > 0))
        break;
    }

    if ((!m_aacheader.config.sample_rate || !m_aacheader.config.channels) && !g_identifying)
      mxerror(fmt::format(FY("The AAC file '{0}' contains invalid header data: the sampling frequency or the number of channels is 0.\n"), m_ti.m_fname));

    m_parser             = mtx::aac::parser_c{};

    int detected_profile = m_aacheader.config.profile;

    if (24000 >= m_aacheader.config.sample_rate)
      m_aacheader.config.profile = mtx::aac::PROFILE_SBR;

    if (   (mtx::includes(m_ti.m_all_aac_is_sbr,  0) && m_ti.m_all_aac_is_sbr[ 0])
        || (mtx::includes(m_ti.m_all_aac_is_sbr, -1) && m_ti.m_all_aac_is_sbr[-1]))
      m_aacheader.config.profile = mtx::aac::PROFILE_SBR;

    if (   (mtx::includes(m_ti.m_all_aac_is_sbr,  0) && !m_ti.m_all_aac_is_sbr[ 0])
        || (mtx::includes(m_ti.m_all_aac_is_sbr, -1) && !m_ti.m_all_aac_is_sbr[-1]))
      m_aacheader.config.profile = detected_profile;

  } catch (...) {
    throw mtx::input::open_x();
  }

  show_demuxer_info();
}

void
aac_reader_c::create_packetizer(int64_t) {
  if (!demuxing_requested('a', 0) || !m_reader_packetizers.empty())
    return;

  auto aacpacketizer = new aac_packetizer_c(this, m_ti, m_aacheader.config, aac_packetizer_c::headerless);
  add_packetizer(aacpacketizer);

  show_packetizer_info(0, *aacpacketizer);
}

file_status_e
aac_reader_c::read(generic_packetizer_c *,
                   bool) {
  auto remaining_bytes = m_size - m_in->getFilePointer();
  auto read_len        = std::min<int64_t>(m_chunk->get_size(), remaining_bytes);
  auto num_read        = m_in->read(m_chunk, read_len);

  if (0 < num_read) {
    m_parser.add_bytes(m_chunk->get_buffer(), num_read);

    while (m_parser.frames_available()) {
      auto frame = m_parser.get_frame();
      ptzr(0).process(std::make_shared<packet_t>(frame.m_data));
    }
  }

  return (0 != num_read) && (0 < (remaining_bytes - num_read)) ? FILE_STATUS_MOREDATA : flush_packetizers();
}

void
aac_reader_c::identify() {
  auto info = mtx::id::info_c{};
  info.add(mtx::id::aac_is_sbr,                      mtx::aac::PROFILE_SBR == m_aacheader.config.profile ? "true"s : "unknown"s);
  info.add(mtx::id::audio_channels,                  m_aacheader.config.channels);
  info.add(mtx::id::audio_sampling_frequency,        m_aacheader.config.sample_rate);
  info.add(mtx::id::audio_output_sampling_frequency, m_aacheader.config.output_sample_rate);

  id_result_container();
  id_result_track(0, ID_RESULT_TRACK_AUDIO, codec_c::get_name(codec_c::type_e::A_AAC, "AAC"), info.get());
}

int
aac_reader_c::find_valid_headers(mm_io_c &in,
                                 int64_t probe_range,
                                 int num_headers) {
  try {
    in.setFilePointer(0);
    auto buf      = memory_c::alloc(probe_range);
    auto num_read = in.read(buf->get_buffer(), probe_range);
    in.setFilePointer(0);

    return mtx::aac::parser_c::find_consecutive_frames(buf->get_buffer(), num_read, num_headers);
  } catch (...) {
    return -1;
  }
}
