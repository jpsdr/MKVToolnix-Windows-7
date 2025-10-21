/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   AV1 Open Bistream Unit (OBU) demultiplexer module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/av1.h"
#include "common/codec.h"
#include "common/dovi_meta.h"
#include "common/endian.h"
#include "common/mm_io_x.h"
#include "common/id_info.h"
#include "input/r_obu.h"
#include "merge/input_x.h"
#include "output/p_av1.h"

static debugging_option_c s_debug_dovi_configuration_record{"dovi_configuration_record"};

bool
obu_reader_c::probe_file() {
  auto size = std::min<uint64_t>(m_in->get_size(), m_buffer->get_size());
  if (m_in->read(m_buffer, size) != size)
    return false;

  mtx::av1::parser_c parser;
  parser.parse(*m_buffer);
  parser.flush();

  if (!parser.headers_parsed())
    return false;

  auto dimensions = parser.get_pixel_dimensions();
  m_width         = dimensions.first;
  m_height        = dimensions.second;

  if ((m_width <= 0) || (m_height <= 0))
    return false;

  if (!parser.has_dovi_rpu_header())
    return true;

  auto hdr                = parser.get_dovi_rpu_header();
  auto color_config       = parser.get_color_config();
  auto frame_duration     = parser.get_frame_duration();

  uint64_t duration       = frame_duration ? mtx::to_int(frame_duration) : 1000000000ll / 25;

  auto dovi_config_record = create_av1_dovi_configuration_record(hdr, m_width, m_height, color_config, duration);

  auto mapping            = mtx::dovi::create_dovi_block_addition_mapping(dovi_config_record);

  if (!mapping.is_valid())
    return true;

  if (s_debug_dovi_configuration_record) {
    hdr.dump();
    dovi_config_record.dump();
  }

  m_block_addition_mappings.push_back(mapping);

  return true;
}

void
obu_reader_c::read_headers() {
  show_demuxer_info();
}

void
obu_reader_c::create_packetizer(int64_t) {
  if (!demuxing_requested('v', 0) || !m_reader_packetizers.empty())
    return;

  auto packetizer = new av1_video_packetizer_c{this, m_ti, m_width, m_height};
  packetizer->set_is_unframed();

  add_packetizer(packetizer);
  ptzr(0).set_block_addition_mappings(m_block_addition_mappings);

  show_packetizer_info(0, *packetizer);
}

file_status_e
obu_reader_c::read(generic_packetizer_c *,
                   bool) {
  try {
    auto to_read = std::min<uint64_t>(m_buffer->get_size(), m_in->get_size() - m_in->getFilePointer());
    if (to_read == 0)
      return flush_packetizers();

    m_in->read(m_buffer, to_read);

    ptzr(0).process(std::make_shared<packet_t>(memory_c::borrow(m_buffer->get_buffer(), to_read)));

    if (to_read == m_buffer->get_size())
      return FILE_STATUS_MOREDATA;

  } catch (mtx::mm_io::exception &) {
  } catch (mtx::av1::exception &) {
  }

  return flush_packetizers();
}

void
obu_reader_c::identify() {
  auto info = mtx::id::info_c{};
  info.add_joined(mtx::id::pixel_dimensions, "x"s, m_width, m_height);

  id_result_container();
  id_result_track(0, ID_RESULT_TRACK_VIDEO, codec_c::get_name(codec_c::type_e::V_AV1, {}), info.get());
}
