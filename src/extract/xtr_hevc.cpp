/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   extracts tracks from Matroska files into other files

*/

#include "common/common_pch.h"

#include "common/endian.h"
#include "common/hacks.h"
#include "common/hevc/util.h"
#include "common/list_utils.h"
#include "extract/xtr_hevc.h"

xtr_hevc_c::xtr_hevc_c(std::string const &codec_id,
                       int64_t tid,
                       track_spec_t &tspec)
  : xtr_avc_c{codec_id, tid, tspec}
  , m_normalize_parameter_sets{!mtx::hacks::is_engaged(mtx::hacks::DONT_NORMALIZE_PARAMETER_SETS)}
{
}

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

  m_parser.set_normalize_parameter_sets(m_normalize_parameter_sets);
  m_parser.set_configuration_record(m_decoded_codec_private);

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
      auto skip_this_nal = skip_prefix_sei && (mtx::hevc::NALU_TYPE_PREFIX_SEI == nal_unit_type);

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
xtr_hevc_c::write_nal(uint8_t *data,
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
  auto start_code_size = m_first_nalu || mtx::included_in(nal_unit_type, mtx::hevc::NALU_TYPE_VIDEO_PARAM, mtx::hevc::NALU_TYPE_SEQ_PARAM, mtx::hevc::NALU_TYPE_PIC_PARAM) ? 4 : 3;
  m_first_nalu         = false;

  m_out->write(ms_start_code + (4 - start_code_size), start_code_size);
  m_out->write(data + pos, nal_size);

  pos += nal_size;

  return true;
}

void
xtr_hevc_c::handle_frame(xtr_frame_t &f) {
  if (   m_first_nalu
      && !m_normalize_parameter_sets
      && (f.frame->get_size() > static_cast<size_t>(m_nal_size_size))) {
    auto nal_unit_type = (f.frame->get_buffer()[m_nal_size_size + 1] >> 1) & 0x3f;
    if (!mtx::included_in(nal_unit_type, mtx::hevc::NALU_TYPE_VIDEO_PARAM, mtx::hevc::NALU_TYPE_SEQ_PARAM, mtx::hevc::NALU_TYPE_PIC_PARAM))
      unwrap_write_hevcc(nal_unit_type == mtx::hevc::NALU_TYPE_PREFIX_SEI);
  }

  if (f.keyframe)
    m_parser.set_next_i_slice_is_key_frame();

  m_parser.add_bytes_framed(f.frame, m_nal_size_size);

  flush_frames();
}

void
xtr_hevc_c::flush_frames() {
  while (m_parser.frame_available()) {
    std::size_t pos{};

    auto frame  = m_parser.get_frame();
    auto buffer = frame.m_data->get_buffer();
    auto size   = frame.m_data->get_size();

    while (pos < size)
      write_nal(buffer, pos, size, m_nal_size_size);
  }
}

void
xtr_hevc_c::finish_track() {
  m_parser.flush();
  flush_frames();
}
