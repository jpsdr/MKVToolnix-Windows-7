/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   VobBtn stream reader

   Written by Steve Lhomme <steve.lhomme@free.fr>.
   Modified by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/codec.h"
#include "common/endian.h"
#include "common/mm_io.h"
#include "common/strings/formatting.h"
#include "common/strings/parsing.h"
#include "input/r_vobbtn.h"
#include "merge/file_status.h"
#include "output/p_vobbtn.h"


bool
vobbtn_reader_c::probe_file() {
  if (m_in->read(chunk, 23) != 23)
    return false;;
  if (strncasecmp(reinterpret_cast<char *>(chunk), "butonDVD", 8))
    return false;;
  if ((0x00 != chunk[0x10]) || (0x00 != chunk[0x11]) || (0x01 != chunk[0x12]) || (0xBF != chunk[0x13]))
    return false;;
  if ((0x03 != chunk[0x14]) || (0xD4 != chunk[0x15]) || (0x00 != chunk[0x16]))
    return false;;
  return true;
}

void
vobbtn_reader_c::read_headers() {
  // read the width & height
  m_in->setFilePointer(8);
  width  = m_in->read_uint16_be();
  height = m_in->read_uint16_be();
  // get ready to read
  m_in->setFilePointer(16);

  show_demuxer_info();
}

void
vobbtn_reader_c::create_packetizer(int64_t tid) {
  m_ti.m_id = tid;
  if (!demuxing_requested('s', tid))
    return;

  add_packetizer(new vobbtn_packetizer_c(this, m_ti, width, height));
  show_packetizer_info(0, ptzr(0));
}

file_status_e
vobbtn_reader_c::read(generic_packetizer_c *,
                      bool) {
  uint8_t tmp[4];

  // _todo_ add some tests on the header and size
  int nread = m_in->read(tmp, 4);
  if (0 >= nread)
    return flush_packetizers();

  uint16_t frame_size = m_in->read_uint16_be();
  nread               = m_in->read(chunk, frame_size);
  if (0 >= nread)
    return flush_packetizers();

  ptzr(0).process(std::make_shared<packet_t>(memory_c::borrow(chunk, nread)));
  return FILE_STATUS_MOREDATA;
}

void
vobbtn_reader_c::identify() {
  id_result_container();
  id_result_track(0, ID_RESULT_TRACK_BUTTONS, codec_c::get_name(codec_c::type_e::B_VOBBTN, "VobBtn"));
}
