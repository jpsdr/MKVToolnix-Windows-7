/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   extracts tracks from Matroska files into other files

*/

#include "common/common_pch.h"

#include "common/endian.h"
#include "common/hevc.h"
#include "common/list_utils.h"
#include "extract/xtr_hevc.h"

void
xtr_hevc_c::create_file(xtr_base_c *master,
                        KaxTrackEntry &track) {
  xtr_base_c::create_file(master, track);

  auto priv = FindChild<KaxCodecPrivate>(&track);
  if (!priv)
    mxerror(boost::format(Y("Track %1% with the CodecID '%2%' is missing the \"codec private\" element and cannot be extracted.\n")) % m_tid % m_codec_id);

  m_decoded_codec_private = decode_codec_private(priv);
  if (m_decoded_codec_private->get_size() < 23)
    mxerror(boost::format(Y("Track %1% CodecPrivate is too small.\n")) % m_tid);

  m_decoded_codec_private->take_ownership();

  unwrap_write_hevcc(false);
}

void
xtr_hevc_c::unwrap_write_hevcc(bool skip_sei) {
  m_out->setFilePointer(0);
  m_out->truncate(0);

  auto buf        = m_decoded_codec_private->get_buffer();
  m_nal_size_size = 1 + (buf[21] & 3);

  // Parameter sets in this order: vps, sps, pps, sei
  auto num_parameter_sets = static_cast<unsigned int>(buf[22]);
  auto pos                = static_cast<size_t>(23);
  auto const priv_size    = m_decoded_codec_private->get_size();

  while (num_parameter_sets && (priv_size > (pos + 3))) {
    auto nal_unit_count  = get_uint16_be(&buf[pos + 1]);
    pos                 += 3;

    while (nal_unit_count && (priv_size > pos)) {
      if ((pos + 2 + 1) > priv_size)
        return;

      auto nal_size      = get_uint_be(&buf[pos], 2);
      auto nal_unit_type = (buf[pos + 2] >> 1) & 0x3f;

      if (skip_sei && (HEVC_NALU_TYPE_PREFIX_SEI == nal_unit_type))
        pos += nal_size;

      else if (!write_nal(buf, pos, priv_size, 2))
        return;

      --nal_unit_count;
      --num_parameter_sets;
    }
  }
}

bool
xtr_hevc_c::write_nal(binary *data,
                      size_t &pos,
                      size_t data_size,
                      size_t write_nal_size_size) {
  if ((pos + write_nal_size_size) > data_size)
    return false;

  auto nal_size  = get_uint_be(&data[pos], write_nal_size_size);
  pos           += write_nal_size_size;

  if ((pos + nal_size) > data_size) {
    mxwarn(boost::format(Y("Track %1%: NAL too big. Size according to header field: %2%, available bytes in packet: %3%. This NAL is defect and will be skipped.\n")) % m_tid % nal_size % (data_size - pos));
    return false;
  }

  auto nal_unit_type   = (data[pos] >> 1) & 0x3f;
  auto start_code_size = m_first_nalu || mtx::included_in(nal_unit_type, HEVC_NALU_TYPE_VIDEO_PARAM, HEVC_NALU_TYPE_SEQ_PARAM, HEVC_NALU_TYPE_PIC_PARAM) ? 4 : 3;
  m_first_nalu         = false;

  m_out->write(ms_start_code + (4 - start_code_size), start_code_size);
  m_out->write(data + pos, nal_size);

  pos += nal_size;

  return true;
}

void
xtr_hevc_c::check_for_sei_in_first_frame(nal_unit_list_t const &nal_units) {
  if (!m_check_for_sei_in_first_frame)
    return;

  for (auto const &nal_unit : nal_units) {
    if (HEVC_NALU_TYPE_PREFIX_SEI != nal_unit.second)
      continue;

    unwrap_write_hevcc(true);
    break;
  }

  m_check_for_sei_in_first_frame = false;
}

void
xtr_hevc_c::handle_frame(xtr_frame_t &f) {
  auto nal_units = find_nal_units(f.frame->get_buffer(), f.frame->get_size());

  check_for_sei_in_first_frame(nal_units);

  for (auto const &nal_unit : nal_units) {
    if (m_drop_vps_sps_pps_in_frame && mtx::included_in(nal_unit.second, HEVC_NALU_TYPE_VIDEO_PARAM, HEVC_NALU_TYPE_SEQ_PARAM, HEVC_NALU_TYPE_PIC_PARAM))
      continue;

    auto &mem = *nal_unit.first;
    auto pos  = static_cast<size_t>(0);

    write_nal(mem.get_buffer(), pos, mem.get_size(), m_nal_size_size);
  }

  m_drop_vps_sps_pps_in_frame = false;
}

unsigned char
xtr_hevc_c::get_nalu_type(unsigned char const *buffer,
                          std::size_t size)
  const {
  return size > 0 ? (buffer[0] >> 1) & 0x3f : 0;
}
