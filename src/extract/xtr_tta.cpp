/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   extracts tracks from Matroska files into other files

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/checksums/base.h"
#include "common/ebml.h"
#include "common/endian.h"
#include "common/mm_io_x.h"
#include "common/mm_file_io.h"
#include "common/mm_proxy_io.h"
#include "common/mm_write_buffer_io.h"
#include "common/tta.h"
#include "extract/xtr_tta.h"

const double xtr_tta_c::ms_tta_frame_time = 1.04489795918367346939l;

xtr_tta_c::xtr_tta_c(const std::string &codec_id,
                     int64_t tid,
                     track_spec_t &tspec)
  : xtr_base_c(codec_id, tid, tspec)
  , m_previous_duration(0)
  , m_bps(0)
  , m_channels(0)
  , m_sfreq(0)
  , m_temp_file_name(fmt::format("mkvextract-{0}-temp-tta-{1}", tid, time(nullptr)))
{
}

void
xtr_tta_c::create_file(xtr_base_c *,
                       libmatroska::KaxTrackEntry &track) {
  try {
    m_out = mm_write_buffer_io_c::open(m_temp_file_name, 5 * 1024 * 1024);
  } catch (mtx::mm_io::exception &ex) {
    mxerror(fmt::format(FY("Failed to create the temporary file '{0}': {1}\n"), m_temp_file_name, ex));
  }

  m_bps      = kt_get_a_bps(track);
  m_channels = kt_get_a_channels(track);
  m_sfreq    = (int)kt_get_a_sfreq(track);
}

void
xtr_tta_c::handle_frame(xtr_frame_t &f) {
  m_frame_sizes.push_back(f.frame->get_size());
  m_out->write(f.frame);

  if (0 < f.duration)
    m_previous_duration = f.duration;
}

void
xtr_tta_c::finish_file() {
  m_out.reset();

  mm_io_cptr in;
  try {
    in = mm_file_io_c::open(m_temp_file_name);
  } catch (mtx::mm_io::exception &ex) {
    mxerror(fmt::format(FY("The temporary file '{0}' could not be opened for reading: {1}.\n"), m_temp_file_name, ex));
  }

  try {
    m_out = mm_write_buffer_io_c::open(m_file_name, 5 * 1024 * 1024);
  } catch (mtx::mm_io::exception &ex) {
    mxerror(fmt::format(FY("The file '{0}' could not be opened for writing: {1}.\n"), m_file_name, ex));
  }

  mtx::tta::file_header_t tta_header;
  memcpy(tta_header.signature, "TTA1", 4);
  if (3 != m_bps)
    put_uint16_le(&tta_header.audio_format, 1);
  else
    put_uint16_le(&tta_header.audio_format, 3);
  put_uint16_le(&tta_header.channels, m_channels);
  put_uint16_le(&tta_header.bits_per_sample, m_bps);
  put_uint32_le(&tta_header.sample_rate, m_sfreq);

  if (0 >= m_previous_duration)
    m_previous_duration = (int64_t)(mtx::tta::FRAME_TIME * m_sfreq) * 1000000000ll;
  put_uint32_le(&tta_header.data_length, (uint32_t)(m_sfreq * (mtx::tta::FRAME_TIME * (m_frame_sizes.size() - 1) + (double)m_previous_duration / 1000000000.0l)));
  put_uint32_le(&tta_header.crc, 0xffffffff ^ mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::crc32_ieee_le, &tta_header, sizeof(mtx::tta::file_header_t) - 4, 0xffffffff));

  m_out->write(&tta_header, sizeof(mtx::tta::file_header_t));

  uint8_t *buffer = (uint8_t *)safemalloc(m_frame_sizes.size() * 4);
  size_t k;
  for (k = 0; m_frame_sizes.size() > k; ++k)
    put_uint32_le(buffer + 4 * k, m_frame_sizes[k]);

  m_out->write(buffer, m_frame_sizes.size() * 4);

  m_out->write_uint32_le(0xffffffff ^ mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::crc32_ieee_le, buffer, m_frame_sizes.size() * 4, 0xffffffff));

  safefree(buffer);

  mxinfo(fmt::format(FY("\nThe temporary TTA file for track ID {0} is being copied into the final TTA file. This may take some time.\n"), m_tid));

  auto mem = memory_c::alloc(128000);
  buffer   = mem->get_buffer();
  int nread;
  do {
    nread = in->read(buffer, 128000);
    m_out->write(buffer, nread);
  } while (nread == 128000);

  m_out.reset();
  in.reset();
  unlink(m_temp_file_name.c_str());
}
