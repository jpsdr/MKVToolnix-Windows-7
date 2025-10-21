/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   HDMV TextST demultiplexer module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/codec.h"
#include "common/endian.h"
#include "common/mm_io_x.h"
#include "common/hdmv_textst.h"
#include "input/r_hdmv_textst.h"
#include "output/p_hdmv_textst.h"
#include "merge/file_status.h"

// 6 bytes magic: "TextST"
// segment descriptor:
//   1 byte segment type (first must be dialog style segment)
//   2 bytes dialog style segment data size
// x bytes dialog style segment data
// 2 bytes number of frames
// remaining: dialog presentation segments including their descriptors

memory_cptr
hdmv_textst_reader_c::read_segment(mm_io_c &in) {
  auto descriptor = in.read(3);
  if (!descriptor || (descriptor->get_size() < 3))
    return {};

  auto segment_size = get_uint16_be(&descriptor->get_buffer()[1]);
  auto segment      = memory_c::alloc(segment_size + 3);

  if (in.read(&segment->get_buffer()[3], segment_size) != segment_size)
    return {};

  std::memcpy(&segment->get_buffer()[0], &descriptor->get_buffer()[0], 3);

  return segment;
}

bool
hdmv_textst_reader_c::probe_file() {
  auto magic = std::string{};
  if ((m_in->read(magic, 6) != 6) || (magic != "TextST"))
    return false;

  auto segment = read_segment(*m_in);
  m_in->skip(2);

  return segment && (mtx::hdmv_textst::dialog_style_segment == segment->get_buffer()[0]) ? 1 : 0;
}

void
hdmv_textst_reader_c::read_headers() {
  m_in->setFilePointer(6);

  m_dialog_style_segment = read_segment(*m_in);

  m_in->skip(2);

  show_demuxer_info();
}

void
hdmv_textst_reader_c::create_packetizer(int64_t) {
  if (!demuxing_requested('s', 0) || !m_reader_packetizers.empty())
    return;

  add_packetizer(new hdmv_textst_packetizer_c(this, m_ti, m_dialog_style_segment));

  show_packetizer_info(0, ptzr(0));
}

file_status_e
hdmv_textst_reader_c::read(generic_packetizer_c *,
                           bool) {
  try {
    auto segment = read_segment(*m_in);
    if (!segment || (segment->get_size() < 13))
      return FILE_STATUS_DONE;

    auto buf    = segment->get_buffer();
    auto start  = mtx::hdmv_textst::get_timestamp(&buf[3]);
    auto end    = mtx::hdmv_textst::get_timestamp(&buf[8]);
    auto packet = std::make_shared<packet_t>(segment, std::min(start, end).to_ns(), (start - end).abs().to_ns());

    ptzr(0).process(packet);

  } catch (...) {
    return flush_packetizers();
  }

  return FILE_STATUS_MOREDATA;
}

void
hdmv_textst_reader_c::identify() {
  id_result_container();
  id_result_track(0, ID_RESULT_TRACK_SUBTITLES, codec_c::get_name(codec_c::type_e::S_HDMV_TEXTST, "HDMV TextST"));
}
