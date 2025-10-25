/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   extracts tracks from Matroska files into other files

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <matroska/KaxTracks.h>

#include "common/codec.h"
#include "common/ebml.h"
#include "common/list_utils.h"
#include "common/mm_io_x.h"
#include "common/mm_proxy_io.h"
#include "common/mm_write_buffer_io.h"
#include "common/strings/editing.h"
#include "extract/xtr_aac.h"
#include "extract/xtr_alac.h"
#include "extract/xtr_avc.h"
#include "extract/xtr_avi.h"
#include "extract/xtr_base.h"
#include "extract/xtr_hdmv_pgs.h"
#include "extract/xtr_hdmv_textst.h"
#include "extract/xtr_hevc.h"
#include "extract/xtr_ivf.h"
#include "extract/xtr_mpeg1_2.h"
#include "extract/xtr_ogg.h"
#include "extract/xtr_rmff.h"
#include "extract/xtr_textsubs.h"
#include "extract/xtr_tta.h"
#include "extract/xtr_vobsub.h"
#include "extract/xtr_wav.h"
#include "extract/xtr_webvtt.h"

xtr_base_c::xtr_base_c(const std::string &codec_id,
                       int64_t tid,
                       track_spec_t &tspec,
                       const char *container_name)
  : m_codec_id(codec_id)
  , m_file_name(tspec.out_name)
  , m_container_name(!container_name ? Y("raw data") : container_name)
  , m_master(nullptr)
  , m_tid(tid)
  , m_track_num(-1)
  , m_default_duration(0)
  , m_bytes_written(0)
  , m_content_decoder_initialized(false)
  , m_debug{}
{
}

xtr_base_c::~xtr_base_c() {
}

void
xtr_base_c::create_file(xtr_base_c *master,
                        libmatroska::KaxTrackEntry &track) {
  auto actual_file_name = get_file_name().string();

  if (master)
    mxerror(fmt::format(FY("Cannot write track {0} with the CodecID '{1}' to the file '{2}' because "
                           "track {3} with the CodecID '{4}' is already being written to the same file.\n"),
                        m_tid, m_codec_id, actual_file_name, master->m_tid, master->m_codec_id));

  try {
    init_content_decoder(track);
    m_out = mm_write_buffer_io_c::open(actual_file_name, 5 * 1024 * 1024);
  } catch (mtx::mm_io::exception &ex) {
    mxerror(fmt::format(FY("Failed to create the file '{0}': {1} ({2})\n"), actual_file_name, errno, ex));
  }

  m_default_duration = kt_get_default_duration(track);
}

void
xtr_base_c::decode_and_handle_frame(xtr_frame_t &f) {
  m_content_decoder.reverse(f.frame, CONTENT_ENCODING_SCOPE_BLOCK);
  handle_frame(f);
}

void
xtr_base_c::handle_frame(xtr_frame_t &f) {
  m_out->write(f.frame);
  m_bytes_written += f.frame->get_size();
}

void
xtr_base_c::finish_track() {
}

void
xtr_base_c::finish_file() {
}

void
xtr_base_c::headers_done() {
}

memory_cptr
xtr_base_c::decode_codec_private(libmatroska::KaxCodecPrivate *priv) {
  auto mpriv = memory_c::borrow(priv->GetBuffer(), priv->GetSize());
  m_content_decoder.reverse(mpriv, CONTENT_ENCODING_SCOPE_CODECPRIVATE);

  return mpriv;
}

void
xtr_base_c::init_content_decoder(libmatroska::KaxTrackEntry &track) {
  if (m_content_decoder_initialized)
    return;

  if (!m_content_decoder.initialize(track))
    mxerror(Y("Tracks with unsupported content encoding schemes (compression or encryption) cannot be extracted.\n"));

  m_content_decoder_initialized = true;
}

std::shared_ptr<xtr_base_c>
xtr_base_c::create_extractor(const std::string &new_codec_id,
                             int64_t new_tid,
                             track_spec_t &tspec) {
  std::shared_ptr<xtr_base_c> result;

  // Raw format
  if (track_spec_t::tm_raw == tspec.target_mode)
    result.reset(new xtr_base_c(new_codec_id, new_tid, tspec));
  else if (track_spec_t::tm_full_raw == tspec.target_mode)
    result.reset(new xtr_fullraw_c(new_codec_id, new_tid, tspec));

  // Audio formats
  else if (new_codec_id == MKV_A_AC3)
    result.reset(new xtr_base_c(new_codec_id, new_tid, tspec, "Dolby Digital (AC-3)"));
  else if (new_codec_id == MKV_A_EAC3)
    result.reset(new xtr_base_c(new_codec_id, new_tid, tspec, "Dolby Digital Plus (E-AC-3)"));
  else if (balg::istarts_with(new_codec_id, "A_MPEG/L"))
    result.reset(new xtr_base_c(new_codec_id, new_tid, tspec, "MPEG-1 Audio Layer 2/3"));
  else if (new_codec_id == MKV_A_DTS)
    result.reset(new xtr_base_c(new_codec_id, new_tid, tspec, "Digital Theater System (DTS)"));
  else if (mtx::included_in(new_codec_id, MKV_A_PCM, MKV_A_PCM_BE))
    result.reset(new xtr_wav_c(new_codec_id, new_tid, tspec));
  else if (new_codec_id == MKV_A_FLAC)
    result.reset(new xtr_flac_c(new_codec_id, new_tid, tspec));
  else if (new_codec_id == MKV_A_ALAC)
    result.reset(new xtr_alac_c(new_codec_id, new_tid, tspec));
  else if (new_codec_id == MKV_A_VORBIS)
    result.reset(new xtr_oggvorbis_c(new_codec_id, new_tid, tspec));
  else if (new_codec_id == MKV_A_OPUS)
    result.reset(new xtr_oggopus_c(new_codec_id, new_tid, tspec));
  else if (balg::istarts_with(new_codec_id, "A_AAC"))
    result.reset(new xtr_aac_c(new_codec_id, new_tid, tspec));
  else if (balg::istarts_with(new_codec_id, "A_REAL/"))
    result.reset(new xtr_rmff_c(new_codec_id, new_tid, tspec));
  else if (new_codec_id == MKV_A_MLP)
    result.reset(new xtr_base_c(new_codec_id, new_tid, tspec, "MLP"));
  else if (new_codec_id == MKV_A_TRUEHD)
    result.reset(new xtr_base_c(new_codec_id, new_tid, tspec, "TrueHD"));
  else if (new_codec_id == MKV_A_TTA)
    result.reset(new xtr_tta_c(new_codec_id, new_tid, tspec));
  else if (new_codec_id == MKV_A_WAVPACK4)
    result.reset(new xtr_wavpack4_c(new_codec_id, new_tid, tspec));

  // Video formats
  else if (new_codec_id == MKV_V_MSCOMP)
    result.reset(new xtr_avi_c(new_codec_id, new_tid, tspec));
  else if (new_codec_id == MKV_V_MPEG4_AVC)
    result.reset(new xtr_avc_c(new_codec_id, new_tid, tspec));
  else if (new_codec_id == MKV_V_MPEGH_HEVC)
    result.reset(new xtr_hevc_c(new_codec_id, new_tid, tspec));
  else if (balg::istarts_with(new_codec_id, "V_REAL/"))
    result.reset(new xtr_rmff_c(new_codec_id, new_tid, tspec));
  else if ((new_codec_id == MKV_V_MPEG1) || (new_codec_id == MKV_V_MPEG2))
    result.reset(new xtr_mpeg1_2_video_c(new_codec_id, new_tid, tspec));
  else if (new_codec_id == MKV_V_THEORA)
    result.reset(new xtr_oggtheora_c(new_codec_id, new_tid, tspec));
  else if ((new_codec_id == MKV_V_VP8) || (new_codec_id == MKV_V_VP9) || (new_codec_id == MKV_V_AV1))
    result.reset(new xtr_ivf_c(new_codec_id, new_tid, tspec));

  // Subtitle formats
  else if ((new_codec_id == MKV_S_TEXTUTF8) || (new_codec_id == MKV_S_TEXTASCII))
    result.reset(new xtr_srt_c(new_codec_id, new_tid, tspec));
  else if ((new_codec_id == MKV_S_TEXTSSA) || (new_codec_id == MKV_S_TEXTASS) || (new_codec_id == "S_SSA") || (new_codec_id == "S_ASS"))
    result.reset(new xtr_ssa_c(new_codec_id, new_tid, tspec));
  else if (new_codec_id == MKV_S_VOBSUB)
    result.reset(new xtr_vobsub_c(new_codec_id, new_tid, tspec));
  else if (new_codec_id == MKV_S_TEXTUSF)
    result.reset(new xtr_usf_c(new_codec_id, new_tid, tspec));
  else if (new_codec_id == MKV_S_KATE)
    result.reset(new xtr_oggkate_c(new_codec_id, new_tid, tspec));
  else if (new_codec_id == MKV_S_HDMV_PGS)
    result.reset(new xtr_hdmv_pgs_c(new_codec_id, new_tid, tspec));
  else if (new_codec_id == MKV_S_HDMV_TEXTST)
    result.reset(new xtr_hdmv_textst_c(new_codec_id, new_tid, tspec));
  else if (new_codec_id == MKV_S_TEXTWEBVTT)
    result.reset(new xtr_webvtt_c(new_codec_id, new_tid, tspec));

  return result;
}

mtx::bcp47::language_c
xtr_base_c::get_track_language(libmatroska::KaxTrackEntry &track) {
  auto language_bcp47 = find_child<libmatroska::KaxLanguageIETF>(&track);
  if (language_bcp47)
    return mtx::bcp47::language_c::parse(language_bcp47->GetValue());

  auto language = find_child<libmatroska::KaxTrackLanguage>(&track);

  return mtx::bcp47::language_c::parse(language ? language->GetValue() : "eng"s);
}

void
xtr_fullraw_c::create_file(xtr_base_c *master,
                           libmatroska::KaxTrackEntry &track) {
  xtr_base_c::create_file(master, track);

  libmatroska::KaxCodecPrivate *priv = find_child<libmatroska::KaxCodecPrivate>(&track);

  if (priv && (0 != priv->GetSize())) {
    auto mem = memory_c::borrow(priv->GetBuffer(), priv->GetSize());
    m_content_decoder.reverse(mem, CONTENT_ENCODING_SCOPE_CODECPRIVATE);
    m_out->write(mem);
  }
}

void
xtr_fullraw_c::handle_codec_state(memory_cptr &codec_state) {
  m_content_decoder.reverse(codec_state, CONTENT_ENCODING_SCOPE_CODECPRIVATE);
  m_out->write(codec_state);
}
