/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   extracts tracks from Matroska files into other files

*/

#include "common/common_pch.h"

#include "common/endian.h"
#include "common/hevc.h"
#include "common/list_utils.h"
#include "extract/xtr_hevc.h"

void
xtr_hevc_c::create_file(xtr_base_c *master,
                        libmatroska::KaxTrackEntry &track) {
  xtr_base_c::create_file(master, track);

  auto priv = FindChild<libmatroska::KaxCodecPrivate>(&track);
  if (!priv)
    mxerror(fmt::format(Y("Track {0} with the CodecID '{1}' is missing the \"codec private\" element and cannot be extracted.\n"), m_tid, m_codec_id));

  m_decoded_codec_private = decode_codec_private(priv);
  if (m_decoded_codec_private->get_size() < 23)
    mxerror(fmt::format(Y("Track {0} CodecPrivate is too small.\n"), m_tid));

  m_decoded_codec_private->take_ownership();

  m_nal_size_size = 1 + (m_decoded_codec_private->get_buffer()[21] & 3);
}

void
xtr_hevc_c::unwrap_write_hevcc(bool skip_prefix_sei) {
  // Parameter sets in this order: vps, sps, pps, sei
  auto buf                = m_decoded_codec_private->get_buffer();
  auto num_parameter_sets = static_cast<unsigned int>(buf[22]);
  auto pos                = static_cast<size_t>(23);
  auto const priv_size    = m_decoded_codec_private->get_size();

  mxdebug_if(m_debug, fmt::format("unwrap_write_hevcc: nal_size_size {0} num_parameter_sets {1}\n", m_nal_size_size, num_parameter_sets));

  while (num_parameter_sets && (priv_size > (pos + 3))) {
    auto nal_unit_count  = get_uint16_be(&buf[pos + 1]);
    pos                 += 3;

    mxdebug_if(m_debug, fmt::format("unwrap_write_hevcc:   num_parameter_sets {0} nal_unit_count {1} pos {2}\n", num_parameter_sets, nal_unit_count, pos - 3));

    while (nal_unit_count && (priv_size > pos)) {
      if ((pos + 2 + 1) > priv_size)
        return;

      auto nal_size      = get_uint_be(&buf[pos], 2);
      auto nal_unit_type = (buf[pos + 2] >> 1) & 0x3f;
      auto skip_this_nal = skip_prefix_sei && (HEVC_NALU_TYPE_PREFIX_SEI == nal_unit_type);

      mxdebug_if(m_debug, fmt::format("unwrap_write_hevcc:     pos {0} nal_size {1} type 0x{2:02x} skip_this_nal {3}\n", pos, nal_size, static_cast<unsigned int>(nal_unit_type), skip_this_nal));

      if (skip_this_nal)
        pos += nal_size + 2;

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
    mxwarn(fmt::format(Y("Track {0}: NAL too big. Size according to header field: {1}, available bytes in packet: {2}. This NAL is defect and will be skipped.\n"), m_tid, nal_size, data_size - pos));
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
xtr_hevc_c::check_for_parameter_sets_in_first_frame(nal_unit_list_t const &nal_units) {
  if (!m_check_for_parameter_sets_in_first_frame)
    return;

  std::unordered_map<unsigned int, bool> nalu_types_found;

  for (auto const &nal_unit : nal_units)
    nalu_types_found[nal_unit.second] = true;

  auto have_all_parameter_sets = nalu_types_found[HEVC_NALU_TYPE_VIDEO_PARAM] && nalu_types_found[HEVC_NALU_TYPE_SEQ_PARAM] && nalu_types_found[HEVC_NALU_TYPE_PIC_PARAM];
  auto have_prefix_sei         = nalu_types_found[HEVC_NALU_TYPE_PREFIX_SEI];

  if (!have_all_parameter_sets)
    unwrap_write_hevcc(have_prefix_sei);

  m_check_for_parameter_sets_in_first_frame = false;
}

void
xtr_hevc_c::handle_frame(xtr_frame_t &f) {
  auto nal_units = find_nal_units(f.frame->get_buffer(), f.frame->get_size());

  check_for_parameter_sets_in_first_frame(nal_units);

  for (auto const &nal_unit : nal_units) {
    auto &mem = *nal_unit.first;
    auto pos  = static_cast<size_t>(0);

    write_nal(mem.get_buffer(), pos, mem.get_size(), m_nal_size_size);
  }
}

unsigned char
xtr_hevc_c::get_nalu_type(unsigned char const *buffer,
                          std::size_t size)
  const {
  return size > 0 ? (buffer[0] >> 1) & 0x3f : 0;
}
