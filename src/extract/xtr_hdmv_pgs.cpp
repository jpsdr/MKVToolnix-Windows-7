/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Extraction of Blu-Ray graphics subtitles.

   Written by Moritz Bunkus and Mike Chen.
*/

#include "common/common_pch.h"

#include <algorithm>
#include <matroska/KaxBlock.h>

#include "common/ebml.h"
#include "common/endian.h"
#include "common/hdmv_pgs.h"
#include "extract/xtr_hdmv_pgs.h"

xtr_hdmv_pgs_c::xtr_hdmv_pgs_c(const std::string &codec_id,
                               int64_t tid,
                               track_spec_t &tspec)
  : xtr_base_c(codec_id, tid, tspec)
{
}

void
xtr_hdmv_pgs_c::handle_frame(xtr_frame_t &f) {
  uint8_t sup_header[10];
  auto mybuffer  = f.frame->get_buffer();
  int frame_size = f.frame->get_size();
  int offset     = 0;
  uint64_t pts   = (f.timestamp * 9) / 100000;

  put_uint16_be(&sup_header[0], mtx::hdmv_pgs::FILE_MAGIC);
  put_uint32_be(&sup_header[2], (uint32_t)pts);
  put_uint32_be(&sup_header[6], 0);

  mxdebug_if(m_debug, fmt::format("frame size {0}\n", frame_size));

  while ((offset + 3) <= frame_size) {
    int segment_size = std::min(static_cast<int>(get_uint16_be(mybuffer + offset + 1) + 3), frame_size - offset);
    auto type        = mybuffer[offset];

    mxdebug_if(m_debug, fmt::format("  segment size {0} at {1} type 0x{2:02x} ({3})\n", segment_size, offset, type, mtx::hdmv_pgs::name_for_type(type)));

    m_out->write(sup_header, 10);
    m_out->write(mybuffer + offset, segment_size);
    offset += segment_size;
  }
}
