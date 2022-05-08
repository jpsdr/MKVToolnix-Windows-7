/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   HEVC/h2.65 ES demultiplexer module

*/

#include "common/common_pch.h"

#include "common/avc_hevc/util.h"
#include "common/byte_buffer.h"
#include "common/codec.h"
#include "common/debugging.h"
#include "common/error.h"
#include "common/memory.h"
#include "common/id_info.h"
#include "common/dovi_meta.h"
#include "input/r_hevc.h"
#include "merge/input_x.h"
#include "merge/file_status.h"
#include "output/p_hevc_es.h"

static debugging_option_c s_debug_dovi_configuration_record{"dovi_configuration_record"};

bool
hevc_es_reader_c::probe_file() {
  // mxinfo(fmt::format("hevc probe {0}\n", m_probe_range_info.require_headers_at_start));
  int num_read, i;
  bool first = true;

  mtx::hevc::es_parser_c parser;
  parser.set_nalu_size_length(4);

  for (i = 0; 50 > i; ++i) {
    num_read = m_in->read(m_buffer->get_buffer(), m_buffer->get_size());
    if (4 > num_read)
      return 0;

    // MPEG TS starts with 0x47.
    if (first && (0x47 == m_buffer->get_buffer()[0]))
      return 0;

    if (first && m_probe_range_info.require_headers_at_start && !mtx::avc_hevc::might_be_avc_or_hevc(*m_buffer))
      return false;

    first = false;

    parser.add_bytes(m_buffer->get_buffer(), num_read);

    if (!parser.headers_parsed())
      continue;

    m_width  = parser.get_width();
    m_height = parser.get_height();

    if (parser.has_stream_default_duration())
      m_default_duration = parser.get_stream_default_duration();

    if (m_block_addition_mappings.empty() && parser.has_dovi_rpu_header()) {
      auto hdr                = parser.get_dovi_rpu_header();
      auto vui                = parser.get_vui_info();
      auto duration           = parser.has_stream_default_duration() ? m_default_duration : parser.get_most_often_used_duration();

      auto dovi_config_record = create_dovi_configuration_record(hdr, m_width, m_height, vui, duration);

      auto mapping            = mtx::dovi::create_dovi_block_addition_mapping(dovi_config_record);

      if (mapping.is_valid()) {
        if (s_debug_dovi_configuration_record) {
          hdr.dump();
          dovi_config_record.dump();
        }

        m_block_addition_mappings.push_back(mapping);
      }
    }

    if ((0 >= m_width) || (0 >= m_height))
      return false;

    return true;
  }

  return false;
}

void
hevc_es_reader_c::read_headers() {
  show_demuxer_info();
}

void
hevc_es_reader_c::create_packetizer(int64_t) {
  if (!demuxing_requested('v', 0) || !m_reader_packetizers.empty())
    return;

  add_packetizer(new hevc_es_video_packetizer_c(this, m_ti));
  ptzr(0).set_video_pixel_dimensions(m_width, m_height);
  ptzr(0).set_block_addition_mappings(m_block_addition_mappings);

  show_packetizer_info(0, ptzr(0));
}

file_status_e
hevc_es_reader_c::read(generic_packetizer_c *,
                      bool) {
  if (m_in->getFilePointer() >= m_size)
    return FILE_STATUS_DONE;

  int num_read = m_in->read(m_buffer->get_buffer(), m_buffer->get_size());
  if (0 < num_read)
    ptzr(0).process(std::make_shared<packet_t>(memory_c::borrow(m_buffer->get_buffer(), num_read)));

  return (0 != num_read) && (m_in->getFilePointer() < m_size) ? FILE_STATUS_MOREDATA : flush_packetizers();
}

void
hevc_es_reader_c::identify() {
  auto info = mtx::id::info_c{};
  info.add(mtx::id::packetizer,       mtx::id::mpegh_p2_es_video);
  info.add(mtx::id::default_duration, m_default_duration);
  info.add_joined(mtx::id::pixel_dimensions, "x"s, m_width, m_height);

  id_result_container();
  id_result_track(0, ID_RESULT_TRACK_VIDEO, codec_c::get_name(codec_c::type_e::V_MPEGH_P2, "HEVC"), info.get());
}
