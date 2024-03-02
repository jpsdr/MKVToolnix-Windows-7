/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Extraction of Blu-Ray text subtitles.

   Written by Moritz Bunkus and Mike Chen.
*/

#include "common/common_pch.h"

#include <algorithm>
#include <matroska/KaxBlock.h>

#include "common/ebml.h"
#include "common/endian.h"
#include "common/hdmv_textst.h"
#include "extract/xtr_hdmv_textst.h"

xtr_hdmv_textst_c::xtr_hdmv_textst_c(const std::string &codec_id,
                               int64_t tid,
                               track_spec_t &tspec)
  : xtr_base_c{codec_id, tid, tspec}
  , m_num_presentation_segments{}
  , m_num_presentation_segment_position{}
{
}

void
xtr_hdmv_textst_c::create_file(xtr_base_c *master,
                               libmatroska::KaxTrackEntry &track) {
  xtr_base_c::create_file(master, track);

  auto priv = find_child<libmatroska::KaxCodecPrivate>(&track);
  if (!priv)
    mxerror(fmt::format(Y("Track {0} with the CodecID '{1}' is missing the \"codec private\" element and cannot be extracted.\n"), m_tid, m_codec_id));

  auto mpriv = decode_codec_private(priv);

  if (mpriv->get_size() < 6)
    mxerror(fmt::format(Y("Track {0} CodecPrivate is too small.\n"), m_tid));

  // Older files created by MakeMKV have a different layout:
  // 1 byte language code
  // segment descriptor:
  //   1 byte segment type (dialog style segment)
  //   2 bytes segment size
  // x bytes dialog style segment data
  // 2 bytes frame count

  // Newer files will only contain the dialog style segment's
  // descriptor and data

  auto buf                 = mpriv->get_buffer();
  auto old_style           = buf[0] && (buf[0] < 0x10);
  auto style_segment_start = old_style ? 1 : 0;
  auto style_segment_size  = get_uint16_be(&buf[style_segment_start + 1]);

  if (mpriv->get_size() < static_cast<uint64_t>(3 + style_segment_size + (old_style ? 1 + 2 : 0)))
    mxerror(fmt::format(Y("Track {0} CodecPrivate is too small.\n"), m_tid));

  m_out->write("TextST"s);

  // The dialog style segment.
  m_out->write(&buf[style_segment_start], 3 + style_segment_size);

  // Remember the position for fixing the number of presentation
  // segments.
  uint8_t const zero[2] = { 0, 0 };
  m_num_presentation_segment_position = m_out->getFilePointer();
  m_out->write(&zero[0], 2);
}

void
xtr_hdmv_textst_c::finish_file() {
  uint8_t buf[2];

  put_uint16_be(&buf[0], m_num_presentation_segments);
  m_out->setFilePointer(m_num_presentation_segment_position);
  m_out->write(&buf[0], 2);
}

void
xtr_hdmv_textst_c::handle_frame(xtr_frame_t &f) {
  if (f.frame->get_size() < 13)
    return;

  ++m_num_presentation_segments;

  auto buf       = f.frame->get_buffer();
  auto start_pts = timestamp_c::ns(f.timestamp);
  auto duration  = std::max<int64_t>(f.duration, 0);
  auto end_pts   = start_pts + (duration != 0 ? timestamp_c::ns(duration) : (mtx::hdmv_textst::get_timestamp(&buf[3 + 5]) - mtx::hdmv_textst::get_timestamp(&buf[3 + 0])).abs());

  mtx::hdmv_textst::put_timestamp(&buf[3 + 0], start_pts);
  mtx::hdmv_textst::put_timestamp(&buf[3 + 5], end_pts);

  m_out->write(f.frame);
}
