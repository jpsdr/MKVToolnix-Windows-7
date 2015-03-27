/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   extracts tracks from Matroska files into other files

*/

#ifndef MTX_XTR_HEVC_CPP
#define MTX_XTR_HEVC_CPP

#include "common/endian.h"
#include "common/hevc.h"
#include "extract/xtr_hevc.h"

void
xtr_hevc_c::create_file(xtr_base_c *master,
                        KaxTrackEntry &track) {
  xtr_base_c::create_file(master, track);

  auto priv = FindChild<KaxCodecPrivate>(&track);
  if (!priv)
    mxerror(boost::format(Y("Track %1% with the CodecID '%2%' is missing the \"codec private\" element and cannot be extracted.\n")) % m_tid % m_codec_id);

  auto mpriv = decode_codec_private(priv);
  if (mpriv->get_size() < 23)
    mxerror(boost::format(Y("Track %1% CodecPrivate is too small.\n")) % m_tid);

  auto buf        = mpriv->get_buffer();
  m_nal_size_size = 1 + (buf[21] & 3);

  // Parameter sets in this order: vps, sps, pps, sei
  auto num_parameter_sets = static_cast<unsigned int>(buf[22]);
  auto pos                = static_cast<size_t>(23);

  while (num_parameter_sets && (mpriv->get_size() > (pos + 3))) {
    auto nal_unit_count  = get_uint16_be(&buf[pos + 1]);
    pos                 += 3;

    while (nal_unit_count && (mpriv->get_size() > pos)) {
      if (!write_nal(buf, pos, mpriv->get_size(), 2))
        return;

      --nal_unit_count;
      --num_parameter_sets;
    }
  }
}

bool
xtr_hevc_c::write_nal(binary const *data,
                      size_t &pos,
                      size_t data_size,
                      size_t write_nal_size_size) {
  if (write_nal_size_size > data_size)
    return false;

  auto nal_size  = get_uint_be(&data[pos], write_nal_size_size);
  pos           += write_nal_size_size;

  if ((pos + nal_size) > data_size) {
    mxwarn(boost::format(Y("Track %1%: NAL too big. Size according to header field: %2%, available bytes in packet: %3%. This NAL is defect and will be skipped.\n")) % m_tid % nal_size % (data_size - pos));
    return false;
  }

  auto nal_unit_type   = (data[pos] >> 1) & 0x3f;
  auto start_code_size = m_first_nalu || (HEVC_NALU_TYPE_VIDEO_PARAM == nal_unit_type) || (HEVC_NALU_TYPE_SEQ_PARAM == nal_unit_type) || (HEVC_NALU_TYPE_PIC_PARAM == nal_unit_type) ? 4 : 3;
  m_first_nalu         = false;

  m_out->write(ms_start_code + (4 - start_code_size), start_code_size);
  m_out->write(data + pos, nal_size);

  pos += nal_size;

  return true;
}

#endif
