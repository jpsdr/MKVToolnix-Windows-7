/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   AV1 Open Bistream Unit (OBU) demultiplexer module

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/av1.h"
#include "common/codec.h"
#include "common/endian.h"
#include "common/mm_io_x.h"
#include "common/id_info.h"
#include "input/r_obu.h"
#include "merge/input_x.h"
#include "output/p_av1.h"

#define READ_SIZE 1024 * 1024

bool
obu_reader_c::probe_file(mm_io_c &in,
                         uint64_t size) {
  try {
    in.setFilePointer(0);
    size     = std::min<uint64_t>(size, READ_SIZE);
    auto buf = in.read(size);

    mtx::av1::parser_c parser;
    parser.parse(*buf);

    if (!parser.headers_parsed() || !parser.frame_available())
      return false;

    auto dimensions = parser.get_pixel_dimensions();
    return (dimensions.first > 0) && (dimensions.second > 0);

  } catch (mtx::mm_io::exception &) {
  } catch (mtx::av1::exception &) {
  }

  return false;
}

obu_reader_c::obu_reader_c(track_info_c const &ti,
                           mm_io_cptr const &in)
  : generic_reader_c{ti, in}
{
}

void
obu_reader_c::read_headers() {
  try {
    m_in->setFilePointer(0);
    m_buffer      = memory_c::alloc(READ_SIZE);
    auto to_read  = std::min<uint64_t>(READ_SIZE, m_in->get_size());

    m_in->read(m_buffer, to_read);

    mtx::av1::parser_c parser;
    parser.parse(m_buffer->get_buffer(), to_read);

    if (!parser.headers_parsed() || !parser.frame_available())
      throw mtx::input::header_parsing_x();

    auto dimensions = parser.get_pixel_dimensions();
    m_width         = dimensions.first;
    m_height        = dimensions.second;

    m_in->setFilePointer(0);

  } catch (mtx::mm_io::exception &) {
    throw mtx::input::header_parsing_x();
  } catch (mtx::av1::exception &) {
    throw mtx::input::header_parsing_x();
  }

  show_demuxer_info();
}

void
obu_reader_c::create_packetizer(int64_t) {
  if (!demuxing_requested('v', 0) || (NPTZR() != 0))
    return;

  auto ptzr = new av1_video_packetizer_c{this, m_ti};

  ptzr->set_is_unframed();
  ptzr->set_video_pixel_width(m_width);
  ptzr->set_video_pixel_height(m_height);

  add_packetizer(ptzr);

  show_packetizer_info(0, ptzr);
}

file_status_e
obu_reader_c::read(generic_packetizer_c *,
                   bool) {
  try {
    auto to_read = std::min<uint64_t>(READ_SIZE, m_in->get_size() - m_in->getFilePointer());
    if (to_read == 0)
      return flush_packetizers();

    m_in->read(m_buffer, to_read);

    PTZR0->process(new packet_t{memory_c::borrow(m_buffer->get_buffer(), to_read)});

    if (to_read == READ_SIZE)
      return FILE_STATUS_MOREDATA;

  } catch (mtx::mm_io::exception &) {
  } catch (mtx::av1::exception &) {
  }

  return flush_packetizers();
}

void
obu_reader_c::identify() {
  auto info = mtx::id::info_c{};
  info.add(mtx::id::pixel_dimensions, boost::format("%1%x%2%") % m_width % m_height);

  id_result_container();
  id_result_track(0, ID_RESULT_TRACK_VIDEO, codec_c::get_name(codec_c::type_e::V_AV1, {}), info.get());
}
