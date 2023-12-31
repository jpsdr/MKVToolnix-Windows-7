/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   extracts tracks from Matroska files into other files
*/

#include "common/common_pch.h"

#include "common/ebml.h"
#include "common/endian.h"
#include "common/list_utils.h"
#include "extract/xtr_avc.h"

uint8_t const xtr_avc_c::ms_start_code[4] = { 0x00, 0x00, 0x00, 0x01 };
uint8_t const xtr_avc_c::ms_aud_nalu[2]   = { 0x09, 0xf0 };

xtr_avc_c::xtr_avc_c(const std::string &codec_id,
                     int64_t tid,
                     track_spec_t &tspec)
  : xtr_base_c(codec_id, tid, tspec)
{
  m_parser.discard_actual_frames(true);
}

bool
xtr_avc_c::write_nal(uint8_t *data,
                     size_t &pos,
                     size_t data_size,
                     size_t write_nal_size_size) {
  size_t i;
  size_t nal_size = 0;

  if (write_nal_size_size > data_size)
    return false;

  for (i = 0; i < write_nal_size_size; ++i)
    nal_size = (nal_size << 8) | data[pos++];

  if ((pos + nal_size) > data_size) {
    mxwarn(fmt::format(Y("Track {0}: NAL too big. Size according to header field: {1}, available bytes in packet: {2}. This NAL is defect and will be skipped.\n"), m_tid, nal_size, data_size - pos));
    return false;
  }

  if (nal_size) {
    m_out->write(ms_start_code, 4);
    m_out->write(data + pos, nal_size);

    pos += nal_size;
  }

  return true;
}

bool
xtr_avc_c::need_to_write_access_unit_delimiter(unsigned char *buffer,
                                               std::size_t size) {
  auto nalu_positions = find_nal_units(buffer, size);
  auto have_aud       = false;

  for (auto const &nalu_pos : nalu_positions) {
    if (static_cast<int>(nalu_pos.first->get_size()) <= m_nal_size_size)
      return false;

    auto nalu = memory_c::borrow(nalu_pos.first->get_buffer() + m_nal_size_size, nalu_pos.first->get_size() - m_nal_size_size);
    auto type = nalu_pos.second;

    mxdebug_if(m_debug_access_unit_delimiters, fmt::format(" type {0}\n", static_cast<unsigned int>(type)));

    if (type == mtx::avc::NALU_TYPE_SEQ_PARAM) {
      m_parser.handle_sps_nalu(nalu);
      continue;
    }

    if (type == mtx::avc::NALU_TYPE_PIC_PARAM) {
      m_parser.handle_pps_nalu(nalu);
      continue;
    }

    if (type == mtx::avc::NALU_TYPE_ACCESS_UNIT) {
      have_aud = true;
      continue;
    }

    if (!mtx::included_in(type, mtx::avc::NALU_TYPE_IDR_SLICE, mtx::avc::NALU_TYPE_NON_IDR_SLICE, mtx::avc::NALU_TYPE_DP_A_SLICE, mtx::avc::NALU_TYPE_DP_B_SLICE, mtx::avc::NALU_TYPE_DP_C_SLICE))
      continue;

    if (have_aud) {
      mxdebug_if(m_debug_access_unit_delimiters, "  AUD before first IDR slice\n");
      return false;
    }

    if (type != mtx::avc::NALU_TYPE_IDR_SLICE) {
      m_previous_idr_pic_id.reset();
      return false;
    }

    mtx::avc_hevc::slice_info_t si;
    if (!m_parser.parse_slice(nalu, si)) {
      mxdebug_if(m_debug_access_unit_delimiters, "  IDR slice parsing failed\n");
      m_previous_idr_pic_id.reset();
      return false;
    }

    mxdebug_if(m_debug_access_unit_delimiters, fmt::format("  IDR slice parsing OK current ID {0} prev ID {1}\n", si.idr_pic_id, m_previous_idr_pic_id ? static_cast<int>(*m_previous_idr_pic_id) : -1));

    auto result           = m_previous_idr_pic_id && (*m_previous_idr_pic_id == si.idr_pic_id);
    m_previous_idr_pic_id = si.idr_pic_id;

    return result;
  }

  return false;
}

void
xtr_avc_c::write_access_unit_delimiter() {
  mxdebug_if(m_debug_access_unit_delimiters, fmt::format("writing access unit delimiter\n"));

  m_out->write(ms_start_code, 4);
  m_out->write(ms_aud_nalu,   2);
}

void
xtr_avc_c::create_file(xtr_base_c *master,
                       libmatroska::KaxTrackEntry &track) {
  xtr_base_c::create_file(master, track);

  auto priv = FindChild<libmatroska::KaxCodecPrivate>(&track);
  if (!priv)
    mxerror(fmt::format(Y("Track {0} with the CodecID '{1}' is missing the \"codec private\" element and cannot be extracted.\n"), m_tid, m_codec_id));

  auto mpriv = decode_codec_private(priv);

  if (mpriv->get_size() < 6)
    mxerror(fmt::format(Y("Track {0} CodecPrivate is too small.\n"), m_tid));

  auto buf        = mpriv->get_buffer();
  m_nal_size_size = 1 + (buf[4] & 3);

  size_t pos          = 6;
  unsigned int numsps = buf[5] & 0x1f;
  size_t i;
  for (i = 0; (i < numsps) && (mpriv->get_size() > pos); ++i) {
    auto previous_pos = pos;
    if (!write_nal(buf, pos, mpriv->get_size(), 2))
      break;

    m_parser.handle_sps_nalu(memory_c::borrow(&buf[previous_pos + 2], pos - previous_pos - 2));
  }

  if (mpriv->get_size() <= pos)
    return;

  unsigned int numpps = buf[pos++];

  for (i = 0; (i < numpps) && (mpriv->get_size() > pos); ++i) {
    auto previous_pos = pos;
    write_nal(buf, pos, mpriv->get_size(), 2);

    m_parser.handle_pps_nalu(memory_c::borrow(&buf[previous_pos + 2], pos - previous_pos - 2));
  }
}

nal_unit_list_t
xtr_avc_c::find_nal_units(uint8_t *buf,
                          std::size_t frame_size)
  const {
  auto list = nal_unit_list_t{};
  auto pos = 0u;

  while (frame_size >= (pos + m_nal_size_size + 1)) {
    auto nal_size              = get_uint_be(&buf[pos], m_nal_size_size);
    auto actual_nal_unit_type  = get_nalu_type(&buf[pos + m_nal_size_size], nal_size);

    list.emplace_back(memory_c::borrow(&buf[pos], m_nal_size_size + nal_size), actual_nal_unit_type);

    pos += m_nal_size_size + nal_size;
  }

  return list;
}

void
xtr_avc_c::handle_frame(xtr_frame_t &f) {
  size_t pos = 0;
  auto buf   = f.frame->get_buffer();

  mxdebug_if(m_debug_access_unit_delimiters, "handle_frame() start\n");

  if (need_to_write_access_unit_delimiter(buf, f.frame->get_size()))
    write_access_unit_delimiter();

  while (f.frame->get_size() > pos)
    if (!write_nal(buf, pos, f.frame->get_size(), m_nal_size_size))
      return;
}

unsigned char
xtr_avc_c::get_nalu_type(unsigned char const *buffer,
                         std::size_t size)
  const {
  return size > 0 ? buffer[0] & 0x1f : 0;
}
