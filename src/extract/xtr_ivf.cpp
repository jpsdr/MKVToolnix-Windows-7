/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   extracts tracks from Matroska files into other files

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/av1.h"
#include "common/codec.h"
#include "common/ebml.h"
#include "common/endian.h"
#include "extract/xtr_ivf.h"

namespace {
unsigned char const s_av1_temporal_delimiter_obu[2] = {
  0x12,                         // type = OBU_TEMPORAL_DELIMITER, extension not present, OBU size field present
  0x00,                         // OBU size
};
}

xtr_ivf_c::xtr_ivf_c(const std::string &codec_id,
                     int64_t tid,
                     track_spec_t &tspec)
  : xtr_base_c(codec_id, tid, tspec)
  , m_is_av1{codec_id == MKV_V_AV1}
{
}

void
xtr_ivf_c::create_file(xtr_base_c *master,
                       libmatroska::KaxTrackEntry &track) {
  xtr_base_c::create_file(master, track);

  uint64_t default_duration = kt_get_default_duration(track);
  default_duration          = 0 == default_duration ? 1 : default_duration;
  uint64_t gcd              = boost::gcd(static_cast<uint64_t>(100000000), default_duration);
  m_frame_rate_num          = 1000000000ull    / gcd;
  m_frame_rate_den          = default_duration / gcd;

  if ((0xffff < m_frame_rate_num) || (0xffff < m_frame_rate_den)) {
    mxdebug_if(m_debug, boost::format("frame rate determination: values too big for 16 bit; default duration %1% numerator %2% denominator %3%\n") % default_duration % m_frame_rate_num % m_frame_rate_den);
    uint64_t scale   = std::max(m_frame_rate_num, m_frame_rate_den) / 0xffff + 1;
    m_frame_rate_num = 1000000000ull    / scale;
    m_frame_rate_den = default_duration / scale;

    if (0 == m_frame_rate_den)
      m_frame_rate_den = 1;
  }

  mxdebug_if(m_debug, boost::format("frame rate determination: default duration %1% numerator %2% denominator %3%\n") % default_duration % m_frame_rate_num % m_frame_rate_den);

  if (master)
    mxerror(boost::format(Y("Cannot write track %1% with the CodecID '%2%' to the file '%3%' because "
                            "track %4% with the CodecID '%5%' is already being written to the same file.\n"))
            % m_tid % m_codec_id % m_file_name % master->m_tid % master->m_codec_id);

  auto fourcc = m_is_av1                ? "AV01"
              : m_codec_id == MKV_V_VP8 ? "VP80"
              :                           "VP90";

  memcpy(m_file_header.file_magic, "DKIF", 4);
  memcpy(m_file_header.fourcc,     fourcc, 4);

  put_uint16_le(&m_file_header.header_size,    sizeof(m_file_header));
  put_uint16_le(&m_file_header.width,          kt_get_v_pixel_width(track));
  put_uint16_le(&m_file_header.height,         kt_get_v_pixel_height(track));
  put_uint16_le(&m_file_header.frame_rate_num, m_frame_rate_num);
  put_uint16_le(&m_file_header.frame_rate_den, m_frame_rate_den);

  m_out->write(&m_file_header, sizeof(m_file_header));
}

void
xtr_ivf_c::handle_frame(xtr_frame_t &f) {
  uint64_t frame_number = f.timestamp * m_frame_rate_num / m_frame_rate_den / 1000000000ull;

  mxdebug_if(m_debug, boost::format("handle frame: timestamp %1% num %2% den %3% frame_number %4% calculated back %5%\n")
             % f.timestamp % m_frame_rate_num % m_frame_rate_den % frame_number
             % (frame_number * 1000000000ull * m_frame_rate_den / m_frame_rate_num));

  av1_prepend_temporal_delimiter_obu_if_needed(*f.frame);

  ivf::frame_header_t frame_header;
  put_uint32_le(&frame_header.frame_size, f.frame->get_size());
  put_uint32_le(&frame_header.timestamp,  frame_number);

  m_out->write(&frame_header,         sizeof(frame_header));
  m_out->write(f.frame->get_buffer(), f.frame->get_size());

  ++m_frame_count;
}

void
xtr_ivf_c::finish_file() {
  put_uint32_le(&m_file_header.frame_count, m_frame_count);

  m_out->setFilePointer(0);
  m_out->write(&m_file_header, sizeof(m_file_header));
}

void
xtr_ivf_c::av1_prepend_temporal_delimiter_obu_if_needed(memory_c &frame) {
  if (!m_is_av1 || !frame.get_size())
    return;

  auto type = (frame.get_buffer()[0] & 0x74) >> 3;

  if (type != mtx::av1::OBU_TEMPORAL_DELIMITER)
    frame.prepend(s_av1_temporal_delimiter_obu, 2);
}
