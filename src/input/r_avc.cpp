/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   AVC/h2.64 ES demultiplexer module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/xyzvc/util.h"
#include "common/codec.h"
#include "common/memory.h"
#include "common/id_info.h"
#include "input/r_avc.h"
#include "merge/file_status.h"
#include "output/p_avc_es.h"

debugging_option_c avc_es_reader_c::ms_debug{"avc_reader"};

bool
avc_es_reader_c::probe_file() {
  // mxinfo(fmt::format("hevc probe {0}\n", m_probe_range_info.require_headers_at_start));
  int num_read, i;
  bool first = true;

  mtx::avc::es_parser_c parser;
  parser.set_nalu_size_length(4);

  for (i = 0; 50 > i; ++i) {
    num_read = m_in->read(m_buffer->get_buffer(), m_buffer->get_size());
    if (4 > num_read)
      return 0;

    // MPEG TS starts with 0x47.
    if (first && (0x47 == m_buffer->get_buffer()[0]))
      return 0;

    if (first && m_probe_range_info.require_headers_at_start && !mtx::xyzvc::might_be_xyzvc(*m_buffer))
      return false;

    first = false;

    parser.add_bytes(m_buffer->get_buffer(), num_read);

    if (!parser.headers_parsed())
      continue;

    m_width  = parser.get_width();
    m_height = parser.get_height();

    if (parser.has_stream_default_duration())
      m_default_duration = parser.get_stream_default_duration();

    if ((0 >= m_width) || (0 >= m_height))
      return false;

    return true;
  }

  return false;
}

void
avc_es_reader_c::read_headers() {
  show_demuxer_info();
}

void
avc_es_reader_c::create_packetizer(int64_t) {
  if (!demuxing_requested('v', 0) || !m_reader_packetizers.empty())
    return;

  add_packetizer(new avc_es_video_packetizer_c(this, m_ti, m_width, m_height));

  show_packetizer_info(0, ptzr(0));
}

file_status_e
avc_es_reader_c::read(generic_packetizer_c *,
                      bool) {
  if (m_in->getFilePointer() >= m_size)
    return FILE_STATUS_DONE;

  int num_read = m_in->read(m_buffer->get_buffer(), m_buffer->get_size());
  if (0 < num_read)
    ptzr(0).process(std::make_shared<packet_t>(memory_c::borrow(m_buffer->get_buffer(), num_read)));

  return (0 != num_read) && (m_in->getFilePointer() < m_size) ? FILE_STATUS_MOREDATA : flush_packetizers();
}

void
avc_es_reader_c::identify() {
  auto info = mtx::id::info_c{};
  info.add(mtx::id::packetizer,       mtx::id::mpeg4_p10_es_video);
  info.add(mtx::id::default_duration, m_default_duration);
  info.add_joined(mtx::id::pixel_dimensions, "x"s, m_width, m_height);

  id_result_container();
  id_result_track(0, ID_RESULT_TRACK_VIDEO, codec_c::get_name(codec_c::type_e::V_MPEG4_P10, "MPEG-4 part 10 ES"), info.get());
}
