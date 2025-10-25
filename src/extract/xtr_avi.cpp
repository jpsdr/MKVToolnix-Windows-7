/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   extracts tracks from Matroska files into other files

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <cmath>

#include "common/ebml.h"
#include "common/endian.h"
#include "common/hacks.h"
#include "common/mm_io_x.h"
#include "common/mm_file_io.h"
#include "common/version.h"
#include "extract/xtr_avi.h"

xtr_avi_c::xtr_avi_c(const std::string &codec_id,
                     int64_t tid,
                     track_spec_t &tspec)
  : xtr_base_c(codec_id, tid, tspec)
  , m_avi(nullptr)
  , m_fps(0.0)
  , m_bih(nullptr)
{
}

void
xtr_avi_c::create_file(xtr_base_c *master,
                       libmatroska::KaxTrackEntry &track) {
  init_content_decoder(track);

  auto priv = find_child<libmatroska::KaxCodecPrivate>(&track);
  if (!priv)
    mxerror(fmt::format(FY("Track {0} with the CodecID '{1}' is missing the \"codec private\" element and cannot be extracted.\n"), m_tid, m_codec_id));

  m_default_duration = kt_get_default_duration(track);
  if (0 >= m_default_duration)
    mxerror(fmt::format(FY("Track {0} with the CodecID '{1}' is missing the \"default duration\" element and cannot be extracted.\n"), m_tid, m_codec_id));

  m_fps = (double)1000000000.0 / (double)m_default_duration;

  if (master)
    mxerror(fmt::format(FY("Cannot write track {0} with the CodecID '{1}' to the file '{2}' because "
                           "track {3} with the CodecID '{4}' is already being written to the same file.\n"),
                        m_tid, m_codec_id, m_file_name, master->m_tid, master->m_codec_id));

  try {
    m_out = mm_file_io_c::open(m_file_name.c_str(), libebml::MODE_CREATE);
    m_avi = AVI_open_output_file(m_out.get());
  } catch (mtx::mm_io::exception &ex) {
    mxerror(fmt::format(FY("The file '{0}' could not be opened for writing: {1}.\n"), m_file_name, ex));
  }

  if (!m_avi)
    mxerror(fmt::format(FY("The file '{0}' could not be opened for writing: {1}.\n"), m_file_name, AVI_strerror()));

  std::string writing_app = "mkvextract";
  if (!mtx::hacks::is_engaged(mtx::hacks::NO_VARIABLE_DATA))
    writing_app += fmt::format(" {0}", get_current_version().to_string());
  m_avi->writing_app = safestrdup(writing_app.c_str());

  char ccodec[5];
  memory_cptr mpriv = decode_codec_private(priv);
  m_bih             = (alBITMAPINFOHEADER *)safememdup(mpriv->get_buffer(), mpriv->get_size());
  memcpy(ccodec, &m_bih->bi_compression, 4);
  ccodec[4]         = 0;

  auto mpriv_size   = mpriv->get_size();

  if (mpriv_size != sizeof(alBITMAPINFOHEADER)) {
    m_avi->extradata      = m_bih + 1;
    m_avi->extradata_size = mpriv_size - std::min(mpriv_size, sizeof(alBITMAPINFOHEADER));
  }

  AVI_set_video(m_avi, kt_get_v_pixel_width(track), kt_get_v_pixel_height(track), get_uint16_le(&m_bih->bi_bit_count), m_fps, ccodec);
}

void
xtr_avi_c::handle_frame(xtr_frame_t &f) {
  AVI_write_frame(m_avi, (char *)f.frame->get_buffer(), f.frame->get_size(), f.keyframe);

  if (((double)f.duration / 1000000.0 - (1000.0 / m_fps)) >= 1.5) {
    int k;
    int nfr = std::llround(static_cast<double>(f.duration) / 1000000.0 * m_fps / 1000.0);
    for (k = 2; k <= nfr; k++)
      AVI_write_frame(m_avi, nullptr, 0, 0);
  }
}

void
xtr_avi_c::finish_file() {
  AVI_close(m_avi);
  safefree(m_bih);
  m_bih = nullptr;
}
