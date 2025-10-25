/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   extracts tracks from Matroska files into other files

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/av1.h"
#include "common/codec.h"
#include "common/ebml.h"
#include "common/endian.h"
#include "common/frame_timing.h"
#include "common/math.h"
#include "extract/xtr_ivf.h"

namespace {
uint8_t const s_av1_temporal_delimiter_obu[2] = {
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

  auto default_duration = find_child_value<libmatroska::KaxTrackDefaultDuration>(track, 1'000'000'000 / 25); // Default to 25 FPS if unknown.
  auto rate             = mtx::frame_timing::determine_frame_rate(default_duration);
  if (!rate)
    rate                = mtx::rational(1'000'000'000ll, default_duration);
  rate                  = mtx::math::clamp_values_to(rate, std::numeric_limits<uint16_t>::max());

  m_frame_rate_num      = static_cast<uint64_t>(boost::multiprecision::numerator(rate));
  m_frame_rate_den      = static_cast<uint64_t>(boost::multiprecision::denominator(rate));

  mxdebug_if(m_debug, fmt::format("frame rate determination: default duration {0} numerator {1} denominator {2}\n", default_duration, m_frame_rate_num, m_frame_rate_den));

  if (master)
    mxerror(fmt::format(FY("Cannot write track {0} with the CodecID '{1}' to the file '{2}' because "
                           "track {3} with the CodecID '{4}' is already being written to the same file.\n"),
                        m_tid, m_codec_id, m_file_name, master->m_tid, master->m_codec_id));

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
  uint64_t frame_number = f.timestamp * m_frame_rate_num / m_frame_rate_den / 1'000'000'000ull;

  mxdebug_if(m_debug,
             fmt::format("handle frame: timestamp {0} num {1} den {2} frame_number {3} calculated back {4}\n",
                         f.timestamp, m_frame_rate_num, m_frame_rate_den, frame_number,
                         frame_number * 1'000'000'000ull * m_frame_rate_den / m_frame_rate_num));

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
