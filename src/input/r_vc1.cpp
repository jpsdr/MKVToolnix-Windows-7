/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   VC1 ES demultiplexer module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/byte_buffer.h"
#include "common/codec.h"
#include "common/endian.h"
#include "common/error.h"
#include "common/id_info.h"
#include "input/r_vc1.h"
#include "merge/input_x.h"
#include "merge/file_status.h"
#include "output/p_vc1.h"


constexpr auto READ_SIZE = 20 * 1024 * 1024;

bool
vc1_es_reader_c::probe_file() {
  auto num_read = m_in->read(m_buffer->get_buffer(), READ_SIZE);

  if (4 > num_read)
    return false;

  uint32_t marker = get_uint32_be(m_buffer->get_buffer());
  if ((mtx::vc1::MARKER_SEQHDR != marker) && (mtx::vc1::MARKER_ENTRYPOINT != marker) && (mtx::vc1::MARKER_FRAME != marker))
    return false;

  mtx::vc1::es_parser_c parser;
  parser.add_bytes(m_buffer->get_buffer(), num_read);

  return parser.is_sequence_header_available();
}

vc1_es_reader_c::vc1_es_reader_c()
  : m_buffer(memory_c::alloc(READ_SIZE))
{
}

void
vc1_es_reader_c::read_headers() {
  try {
    mtx::vc1::es_parser_c parser;

    int num_read = m_in->read(m_buffer->get_buffer(), READ_SIZE);
    parser.add_bytes(m_buffer->get_buffer(), num_read);

    if (!parser.is_sequence_header_available())
      throw false;

    parser.get_sequence_header(m_seqhdr);

    m_in->setFilePointer(0);

  } catch (...) {
    throw mtx::input::open_x();
  }

  show_demuxer_info();
}

void
vc1_es_reader_c::create_packetizer(int64_t) {
  if (!demuxing_requested('v', 0) || !m_reader_packetizers.empty())
    return;

  add_packetizer(new vc1_video_packetizer_c(this, m_ti));

  show_packetizer_info(0, ptzr(0));
}

file_status_e
vc1_es_reader_c::read(generic_packetizer_c *,
                      bool) {
  if (m_in->getFilePointer() >= m_size)
    return flush_packetizers();

  int num_read = m_in->read(m_buffer->get_buffer(), READ_SIZE);
  if (0 < num_read)
    ptzr(0).process(std::make_shared<packet_t>(memory_c::borrow(m_buffer->get_buffer(), num_read)));

  return ((READ_SIZE != num_read) || (m_in->getFilePointer() >= m_size)) ? flush_packetizers() : FILE_STATUS_MOREDATA;
}

void
vc1_es_reader_c::identify() {
  auto info = mtx::id::info_c{};
  info.add_joined(mtx::id::pixel_dimensions, "x"s, m_seqhdr.pixel_width, m_seqhdr.pixel_height);

  id_result_container();
  id_result_track(0, ID_RESULT_TRACK_VIDEO, codec_c::get_name(codec_c::type_e::V_VC1, "VC1"), info.get());
}
