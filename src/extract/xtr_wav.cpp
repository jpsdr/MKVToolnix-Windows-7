/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   extracts tracks from Matroska files into other files

   Written by Moritz Bunkus <mo@bunkus.online>
   and Steve Lhomme <steve.lhomme@free.fr>.
*/

#include "common/common_pch.h"

#include <QRegularExpression>

#include <matroska/KaxBlock.h>

#include "avilib.h"
#include "common/bswap.h"
#include "common/codec.h"
#include "common/ebml.h"
#include "common/endian.h"
#include "common/list_utils.h"
#include "common/mm_io_x.h"
#include "common/mm_proxy_io.h"
#include "common/mm_write_buffer_io.h"
#include "common/qt.h"
#include "common/w64.h"
#include "extract/xtr_wav.h"

xtr_wav_c::xtr_wav_c(const std::string &codec_id,
                     int64_t tid,
                     track_spec_t &tspec)
  : xtr_base_c(codec_id, tid, tspec)
{
}

void
xtr_wav_c::create_file(xtr_base_c *master,
                       libmatroska::KaxTrackEntry &track) {
  init_content_decoder(track);

  m_w64_requested = Q(get_file_name()).contains(QRegularExpression{"\\.[wW]64$"});
  m_channels      = kt_get_a_channels(track);
  m_sfreq         = kt_get_a_sfreq(track);
  m_bps           = kt_get_a_bps(track);

  if (-1 == m_bps)
    mxerror(fmt::format(FY("Track {0} with the CodecID '{1}' is missing the \"bits per second (bps)\" element and cannot be extracted.\n"), m_tid, m_codec_id));

  if (m_codec_id == MKV_A_PCM_BE)
    m_byte_swapper = [this](uint8_t const *src, uint8_t *dst, std::size_t num_bytes) {
      mtx::bytes::swap_buffer(src, dst, num_bytes, m_bps / 8);
    };

  xtr_base_c::create_file(master, track);

  write_w64_header();
}

void
xtr_wav_c::write_wav_header() {
  auto junk_size   = m_w64_header_size - sizeof(wave_header);
  auto junk_chunk  = memory_c::alloc(junk_size);
  auto junk_buffer = junk_chunk->get_buffer();

  std::memcpy(  &junk_buffer[0], "JUNK", 4);
  put_uint32_le(&junk_buffer[4], junk_size - 8);
  std::memset(  &junk_buffer[8], 0, junk_size - 8);

  wave_header wh;

  std::memset(&wh, 0, sizeof(wh));

  memcpy(&wh.riff.id,      "RIFF", 4);
  memcpy(&wh.riff.wave_id, "WAVE", 4);
  memcpy(&wh.format.id,    "fmt ", 4);
  memcpy(&wh.data.id,      "data", 4);

  put_uint32_le(&wh.riff.len,                m_bytes_written + m_w64_header_size - 8);
  put_uint32_le(&wh.data.len,                m_bytes_written);
  put_uint32_le(&wh.format.len,              16);
  put_uint16_le(&wh.common.wFormatTag,        1);
  put_uint16_le(&wh.common.wChannels,        m_channels);
  put_uint32_le(&wh.common.dwSamplesPerSec,  m_sfreq);
  put_uint32_le(&wh.common.dwAvgBytesPerSec, m_channels * m_sfreq * m_bps / 8);
  put_uint16_le(&wh.common.wBlockAlign,      m_bps * m_channels / std::gcd(8, m_bps));
  put_uint16_le(&wh.common.wBitsPerSample,   m_bps);

  m_out->setFilePointer(0);
  m_out->write(&wh.riff,   sizeof(wh.riff));
  m_out->write(junk_chunk);
  m_out->write(&wh.format, sizeof(wh.format));
  m_out->write(&wh.common, sizeof(wh.common));
  m_out->write(&wh.data,   sizeof(wh.data));
}

void
xtr_wav_c::write_w64_header() {
  wave_header wh;

  std::memset(&wh, 0, sizeof(wh));

  put_uint16_le(&wh.common.wFormatTag,       1);
  put_uint16_le(&wh.common.wChannels,        m_channels);
  put_uint32_le(&wh.common.dwSamplesPerSec,  m_sfreq);
  put_uint32_le(&wh.common.dwAvgBytesPerSec, m_channels * m_sfreq * m_bps / 8);
  put_uint16_le(&wh.common.wBlockAlign,      m_bps * m_channels / std::gcd(8, m_bps));
  put_uint16_le(&wh.common.wBitsPerSample,   m_bps);

  m_out->setFilePointer(0);

  m_out->write(mtx::w64::g_guid_riff, 16);
  m_out->write_uint64_le(m_bytes_written ? m_bytes_written + m_w64_header_size : 0); // TODO
  m_out->write(mtx::w64::g_guid_wave, 16);
  m_out->write(mtx::w64::g_guid_fmt,  16);
  m_out->write_uint64_le(sizeof(wh.common) + 16 + 8);
  m_out->write(&wh.common, sizeof(wh.common));
  m_out->write(mtx::w64::g_guid_data, 16);
  m_out->write_uint64_le(m_bytes_written ? m_bytes_written + 16 + 8 : 0);

  m_w64_header_size = m_out->getFilePointer();
}

void
xtr_wav_c::finish_file() {
  auto too_big = (m_bytes_written + m_w64_header_size) > static_cast<uint64_t>(std::numeric_limits<uint32_t>::max());

  if (!too_big && !m_w64_requested) {
    write_wav_header();
    return;
  }

  if (!m_w64_requested)
    mxinfo(fmt::format(FY("The file '{0}' was written as a W64 file instead of WAV as it is bigger than 4 GB and therefore too big to fit into the WAV container.\n"), get_file_name().string()));

  write_w64_header();
}

void
xtr_wav_c::handle_frame(xtr_frame_t &f) {
  if (m_byte_swapper)
    m_byte_swapper(f.frame->get_buffer(), f.frame->get_buffer(), f.frame->get_size());

  m_out->write(f.frame);
  m_bytes_written += f.frame->get_size();
}

char const *
xtr_wav_c::get_container_name() {
  return m_w64_requested ? "W64" : "WAV";
}

// ------------------------------------------------------------------------

xtr_wavpack4_c::xtr_wavpack4_c(const std::string &codec_id,
                               int64_t tid,
                               track_spec_t &tspec)
  : xtr_base_c(codec_id, tid, tspec)
  , m_number_of_samples(0)
  , m_extract_blockadd_level(tspec.extract_blockadd_level)
{
}

void
xtr_wavpack4_c::create_file(xtr_base_c *master,
                            libmatroska::KaxTrackEntry &track) {
  memory_cptr mpriv;

  init_content_decoder(track);

  auto priv = find_child<libmatroska::KaxCodecPrivate>(&track);
  if (priv)
    mpriv = decode_codec_private(priv);

  if (!priv || (2 > mpriv->get_size()))
    mxerror(fmt::format(FY("Track {0} with the CodecID '{1}' is missing the \"codec private\" element and cannot be extracted.\n"), m_tid, m_codec_id));
  memcpy(m_version, mpriv->get_buffer(), 2);

  xtr_base_c::create_file(master, track);

  m_channels = kt_get_a_channels(track);

  if ((0 != kt_get_max_blockadd_id(track)) && (0 != m_extract_blockadd_level)) {
    std::string corr_name = m_file_name;
    size_t pos            = corr_name.rfind('.');

    if ((std::string::npos != pos) && (0 != pos))
      corr_name.erase(pos + 1);
    corr_name += "wvc";

    try {
      m_corr_out = mm_write_buffer_io_c::open(corr_name, 5 * 1024 * 1024);
    } catch (mtx::mm_io::exception &ex) {
      mxerror(fmt::format(FY("The file '{0}' could not be opened for writing: {1}.\n"), corr_name, ex));
    }
  }
}

void
xtr_wavpack4_c::handle_frame(xtr_frame_t &f) {
  // build the main header

  uint8_t wv_header[32];
  memcpy(wv_header, "wvpk", 4);
  memcpy(&wv_header[8], m_version, 2); // version
  wv_header[10] = 0;                   // track_no
  wv_header[11] = 0;                   // index_no
  wv_header[12] = 0xFF;                // total_samples is unknown
  wv_header[13] = 0xFF;
  wv_header[14] = 0xFF;
  wv_header[15] = 0xFF;
  put_uint32_le(&wv_header[16], m_number_of_samples); // block_index

  auto mybuffer        = f.frame->get_buffer();
  int data_size        = f.frame->get_size();
  int truncate_bytes   = 0;
  m_number_of_samples += get_uint32_le(mybuffer);

  // rest of the header:
  memcpy(&wv_header[20], mybuffer, 3 * sizeof(uint32_t));

  std::vector<uint32_t> flags;

  // support multi-track files
  if (2 < m_channels) {
    uint32_t block_size = get_uint32_le(&mybuffer[12]);

    // For now, we are deleting any block checksums that might be present in the WavPack data. They can't be verified
    // because the original 32-byte header is gone (which is included in the checksum) and they will probably be wrong
    // if we write them here because we are creating new headers. This also applies to the correction blocks below.

    truncate_bytes = (get_uint32_le(&wv_header[24]) & mtx::wavpack::HAS_CHECKSUM) ? mtx::wavpack::checksum_byte_count(mybuffer + 16, block_size) : 0;
    put_uint32_le(&wv_header[24], get_uint32_le(&wv_header[24]) & ~mtx::wavpack::HAS_CHECKSUM);

    put_uint32_le(&wv_header[4], block_size + 24 - truncate_bytes);  // ck_size
    m_out->write(wv_header, 32);
    flags.push_back(get_uint32_le(&mybuffer[4]));
    mybuffer += 16;
    m_out->write(mybuffer, block_size - truncate_bytes);
    mybuffer  += block_size;
    data_size -= block_size + 16;
    while (0 < data_size) {
      block_size = get_uint32_le(&mybuffer[8]);
      memcpy(&wv_header[24], mybuffer, 8);

      truncate_bytes = (get_uint32_le(&wv_header[24]) & mtx::wavpack::HAS_CHECKSUM) ? mtx::wavpack::checksum_byte_count (mybuffer + 12, block_size) : 0;
      put_uint32_le(&wv_header[24], get_uint32_le(&wv_header[24]) & ~mtx::wavpack::HAS_CHECKSUM);

      put_uint32_le(&wv_header[4], block_size + 24 - truncate_bytes);
      m_out->write(wv_header, 32);

      flags.push_back(get_uint32_le(mybuffer));
      mybuffer += 12;
      m_out->write(mybuffer, block_size - truncate_bytes);

      mybuffer  += block_size;
      data_size -= block_size + 12;
    }

  } else {
    truncate_bytes = (get_uint32_le(&wv_header[24]) & mtx::wavpack::HAS_CHECKSUM) ? mtx::wavpack::checksum_byte_count (mybuffer + 12, data_size - 12) : 0;
    put_uint32_le (&wv_header[24], get_uint32_le(&wv_header[24]) & ~mtx::wavpack::HAS_CHECKSUM);

    put_uint32_le(&wv_header[4], data_size + 12 - truncate_bytes); // ck_size
    m_out->write(wv_header, 32);
    m_out->write(&mybuffer[12], data_size - 12 - truncate_bytes); // the rest of the
  }

  // support hybrid mode data
  if (m_corr_out && (f.additions)) {
    auto block_more = find_child<libmatroska::KaxBlockMore>(f.additions);

    if (!block_more)
      return;

    auto block_addition = find_child<libmatroska::KaxBlockAdditional>(block_more);
    if (!block_addition)
      return;

    data_size = block_addition->GetSize();
    mybuffer  = block_addition->GetBuffer();

    if (2 < m_channels) {
      size_t flags_index = 0;

      while (0 < data_size) {
        uint32_t block_size = get_uint32_le(&mybuffer[4]);

        memcpy(&wv_header[24], &flags[flags_index++], 4); // flags
        memcpy(&wv_header[28], mybuffer, 4); // crc

        truncate_bytes = (get_uint32_le(&wv_header[24]) & mtx::wavpack::HAS_CHECKSUM) ? mtx::wavpack::checksum_byte_count (mybuffer + 8, block_size) : 0;
        put_uint32_le (&wv_header[24], get_uint32_le(&wv_header[24]) & ~mtx::wavpack::HAS_CHECKSUM);

        put_uint32_le(&wv_header[4], block_size + 24 - truncate_bytes); // ck_size
        m_corr_out->write(wv_header, 32);
        mybuffer += 8;
        m_corr_out->write(mybuffer, block_size - truncate_bytes);
        mybuffer += block_size;
        data_size -= 8 + block_size;
      }

    } else {
      truncate_bytes = truncate_bytes ? mtx::wavpack::checksum_byte_count (mybuffer + 4, data_size - 4) : 0;
      put_uint32_le(&wv_header[4], data_size + 20 - truncate_bytes); // ck_size
      memcpy(&wv_header[28], mybuffer, 4); // crc
      m_corr_out->write(wv_header, 32);
      m_corr_out->write(&mybuffer[4], data_size - 4 - truncate_bytes);
    }
  }
}

void
xtr_wavpack4_c::finish_file() {
  m_out->setFilePointer(12);
  m_out->write_uint32_le(m_number_of_samples);

  if (m_corr_out) {
    m_corr_out->setFilePointer(12);
    m_corr_out->write_uint32_le(m_number_of_samples);
  }
}
