/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   WAV reader module

   Written by Moritz Bunkus <mo@bunkus.online>.
   Initial DTS support by Peter Niemayer <niemayer@isg.de> and
     modified by Moritz Bunkus.
*/

#include "common/common_pch.h"

#include "avilib.h"
#include "common/ac3.h"
#include "common/dts.h"
#include "common/dts_parser.h"
#include "common/endian.h"
#include "common/id_info.h"
#include "common/mm_io_x.h"
#include "common/strings/formatting.h"
#include "common/w64.h"
#include "input/r_wav.h"
#include "input/wav_ac3acm_demuxer.h"
#include "input/wav_ac3wav_demuxer.h"
#include "input/wav_dts_demuxer.h"
#include "input/wav_pcm_demuxer.h"
#include "merge/input_x.h"

wav_demuxer_c::wav_demuxer_c(wav_reader_c *reader,
                             wave_header *wheader)
  : m_reader(reader)
  , m_wheader(wheader)
  , m_ptzr(nullptr)
  , m_ti(reader->m_ti)
{
}

// ----------------------------------------------------------

wav_reader_c::type_e
wav_reader_c::determine_type() {
  if (static_cast<int64_t>(sizeof(mtx::w64::header_t)) > m_in->get_size())
    return type_e::unknown;

  try {
    wave_header wheader;
    mtx::w64::header_t w64_header;

    m_in->setFilePointer(0);

    if (m_in->read(&w64_header, sizeof(w64_header)) != sizeof(w64_header))
      return type_e::unknown;

    m_in->setFilePointer(0);

    std::memcpy(&wheader.riff, &w64_header, sizeof(wheader.riff));

    if (   !std::memcmp(&wheader.riff.id,      "RIFF", 4)
        && !std::memcmp(&wheader.riff.wave_id, "WAVE", 4))
      return type_e::wave;

    if (   !std::memcmp(&wheader.riff.id,      "RF64", 4)
        && !std::memcmp(&wheader.riff.wave_id, "WAVE", 4))
      return type_e::rf64;

    if (   !std::memcmp(w64_header.riff.guid, mtx::w64::g_guid_riff, 16)
        && !std::memcmp(w64_header.wave_guid, mtx::w64::g_guid_wave, 16))
      return type_e::wave64;

  } catch (mtx::mm_io::exception &) {
    return type_e::unknown;
  }

  return type_e::unknown;
}

bool
wav_reader_c::probe_file() {
  m_type = determine_type();
  return m_type != type_e::unknown;
}

void
wav_reader_c::read_headers() {
  parse_file();
  create_demuxer();
}

void
wav_reader_c::parse_fmt_chunk() {
  auto chunk_idx = find_chunk("fmt ");

  if (!chunk_idx)
    throw mtx::input::header_parsing_x();

  try {
    m_in->setFilePointer(m_chunks[*chunk_idx].pos);

    if (static_cast<uint64_t>(m_chunks[*chunk_idx].len) >= sizeof(alWAVEFORMATEXTENSIBLE)) {
      alWAVEFORMATEXTENSIBLE format;
      if (m_in->read(&format, sizeof(format)) != sizeof(format))
        throw false;
      memcpy(&m_wheader.common, &format, sizeof(m_wheader.common));

      m_format_tag = get_uint16_le(&m_wheader.common.wFormatTag);
      if (0xfffe == m_format_tag)
        m_format_tag = get_uint32_le(&format.extension.guid.data1);

    } else if (m_in->read(&m_wheader.common, sizeof(m_wheader.common)) != sizeof(m_wheader.common))
      throw false;

    else
      m_format_tag = get_uint16_le(&m_wheader.common.wFormatTag);

  } catch (...) {
    throw mtx::input::header_parsing_x();
  }
}

void
wav_reader_c::parse_file() {
  scan_chunks();

  parse_fmt_chunk();

  m_cur_data_chunk_idx = find_chunk("data", 0, false);
  if (!m_cur_data_chunk_idx)
    throw mtx::input::header_parsing_x();

  if (debugging_c::requested("wav_reader|wav_reader_headers"))
    dump_headers();

  m_in->setFilePointer(m_chunks[*m_cur_data_chunk_idx].pos);

  m_remaining_bytes_in_current_data_chunk = m_chunks[*m_cur_data_chunk_idx].len;
}

void
wav_reader_c::dump_headers() {
  mxinfo(fmt::format("File '{0}' header dump (mode: {1})\n", m_ti.m_fname, m_type == type_e::rf64 ? "RF64" : m_type == type_e::wave ? "WAV" : "Wave64"));

  if ((m_type == type_e::rf64) || (m_type == type_e::wave))
    mxinfo(fmt::format("  riff:\n"
                       "    id:      {0}{1}{2}{3}\n"
                       "    len:     {4}\n"
                       "    wave_id: {5}{6}{7}{8}\n",
                       char(m_wheader.riff.id[0]), char(m_wheader.riff.id[1]), char(m_wheader.riff.id[2]), char(m_wheader.riff.id[3]),
                       get_uint32_le(&m_wheader.riff.len),
                       char(m_wheader.riff.wave_id[0]), char(m_wheader.riff.wave_id[1]), char(m_wheader.riff.wave_id[2]), char(m_wheader.riff.wave_id[3])));

  if (m_type == type_e::rf64)
    mxinfo(fmt::format("  ds64:\n"
                       "    riff_size:    {0}\n"
                       "    data_size:    {1}\n"
                       "    sample_count: {2}\n",
                       m_ds64.riff_size, m_ds64.data_size, m_ds64.sample_count));

  mxinfo(fmt::format("  common:\n"
                     "    wFormatTag:       {0:04x}\n"
                     "    wChannels:        {1}\n"
                     "    dwSamplesPerSec:  {2}\n"
                     "    dwAvgBytesPerSec: {3}\n"
                     "    wBlockAlign:      {4}\n"
                     "    wBitsPerSample:   {5}\n"
                     "  actual format_tag:  {6}\n",
                     get_uint16_le(&m_wheader.common.wFormatTag),
                     get_uint16_le(&m_wheader.common.wChannels),
                     get_uint32_le(&m_wheader.common.dwSamplesPerSec),
                     get_uint32_le(&m_wheader.common.dwAvgBytesPerSec),
                     get_uint16_le(&m_wheader.common.wBlockAlign),
                     get_uint16_le(&m_wheader.common.wBitsPerSample),
                     m_format_tag));
}

void
wav_reader_c::create_demuxer() {
  if (0x2000 == m_format_tag) {
    m_demuxer = wav_demuxer_cptr(new wav_ac3acm_demuxer_c(this, &m_wheader));
    if (!m_demuxer->probe(m_in))
      m_demuxer.reset();
  }

  if (!m_demuxer) {
    m_demuxer = wav_demuxer_cptr(new wav_dts_demuxer_c(this, &m_wheader));
    if (!m_demuxer->probe(m_in))
      m_demuxer.reset();
  }

  if (!m_demuxer) {
    m_demuxer = wav_demuxer_cptr(new wav_ac3wav_demuxer_c(this, &m_wheader));
    if (!m_demuxer->probe(m_in))
      m_demuxer.reset();
  }

  if (!m_demuxer && ((0x0001 == m_format_tag) || (0x0003 == m_format_tag)))
    m_demuxer = wav_demuxer_cptr(new wav_pcm_demuxer_c(this, &m_wheader, 0x00003 == m_format_tag));

  show_demuxer_info();
}

void
wav_reader_c::create_packetizer(int64_t) {
  if (!demuxing_requested('a', 0) || !m_reader_packetizers.empty() || !m_demuxer)
    return;

  add_packetizer(m_demuxer->create_packetizer());
}

file_status_e
wav_reader_c::read(generic_packetizer_c *,
                   bool) {
  if (!m_demuxer)
    return FILE_STATUS_DONE;

  auto requested_bytes = std::min(m_remaining_bytes_in_current_data_chunk, m_demuxer->get_preferred_input_size());
  auto buffer          = m_demuxer->get_buffer();
  auto num_read        = m_in->read(buffer, requested_bytes);

  if (0 >= num_read)
    return flush_packetizers();

  m_demuxer->process(num_read);

  m_remaining_bytes_in_current_data_chunk -= num_read;

  if (!m_remaining_bytes_in_current_data_chunk) {
    m_cur_data_chunk_idx = find_chunk("data", *m_cur_data_chunk_idx + 1, false);

    if (!m_cur_data_chunk_idx)
      return flush_packetizers();

    m_in->setFilePointer(m_chunks[*m_cur_data_chunk_idx].pos);

    m_remaining_bytes_in_current_data_chunk = m_chunks[*m_cur_data_chunk_idx].len;
  }

  return FILE_STATUS_MOREDATA;
}

void
wav_reader_c::scan_chunks() {
  if (m_type == type_e::rf64)
    scan_chunks_rf64();
  else if (m_type == type_e::wave)
    scan_chunks_wave();
  else
    scan_chunks_wave64();
}

void
wav_reader_c::scan_chunks_rf64() {
  scan_chunks_wave();

  auto chunk_idx    = find_chunk("ds64");
  auto debug_chunks = debugging_c::requested("wav_reader|wav_reader_chunks");

  if (!chunk_idx) {
    mxdebug_if(debug_chunks, fmt::format("scan_chunks_rf64() no ds64 chunk found\n"));
    throw mtx::input::header_parsing_x();
  }

  auto const &chunk = m_chunks[*chunk_idx];

  mxdebug_if(debug_chunks, fmt::format("scan_chunks_rf64() found ds64 at {0} size {1}\n", chunk.pos, chunk.len));

  if (chunk.len < 24)
    throw mtx::input::header_parsing_x();

  m_in->setFilePointer(chunk.pos);

  m_ds64.riff_size       = m_in->read_uint64_le();
  m_ds64.data_size       = m_in->read_uint64_le();
  m_ds64.sample_count    = m_in->read_uint64_le();

  m_bytes_in_data_chunks = m_ds64.data_size;

  chunk_idx              = find_chunk("data");

  if (chunk_idx)
    m_chunks[*chunk_idx].len = m_ds64.data_size;
}

void
wav_reader_c::scan_chunks_wave() {
  m_in->setFilePointer(0);

  if (m_in->read(&m_wheader.riff, sizeof(m_wheader.riff)) != sizeof(m_wheader.riff))
    throw mtx::input::header_parsing_x();

  wav_chunk_t new_chunk;
  bool debug_chunks = debugging_c::requested("wav_reader|wav_reader_chunks");

  mxdebug_if(debug_chunks, fmt::format("scan_chunks_wave() Starting search at file position {0}\n", m_in->getFilePointer()));

  try {
    auto file_size = m_in->get_size();
    char id[4];

    while (true) {
      new_chunk.pos = m_in->getFilePointer() + 8;

      if (m_in->read(id, 4) != 4)
        return;

      new_chunk.id  = memory_c::clone(id, 4);
      new_chunk.len = m_in->read_uint32_le();

      mxdebug_if(debug_chunks, fmt::format("scan_chunks_wave() new chunk at {0} type {1} length {2}\n", new_chunk.pos - 8, mtx::string::get_displayable(id, 4), new_chunk.len));

      if (!strncasecmp(id, "data", 4))
        m_bytes_in_data_chunks += new_chunk.len;

      else if (   !m_chunks.empty()
               && !strncasecmp(reinterpret_cast<char *>(m_chunks.back().id->get_buffer()), "data", 4)
               && (file_size > 0x100000000ll)) {
        wav_chunk_t &previous_chunk  = m_chunks.back();
        int64_t this_chunk_len       = file_size - previous_chunk.pos;
        m_bytes_in_data_chunks      -= previous_chunk.len;
        m_bytes_in_data_chunks      += this_chunk_len;
        previous_chunk.len           = this_chunk_len;

        mxdebug_if(debug_chunks, fmt::format("scan_chunks_wave() huge data chunk with wrong length at {0}; re-calculated from file size; new length {1}\n", previous_chunk.pos, previous_chunk.len));

        break;
      }

      m_chunks.push_back(new_chunk);
      m_in->setFilePointer(new_chunk.len, libebml::seek_current);

    }
  } catch (...) {
  }
}

void
wav_reader_c::scan_chunks_wave64() {
  wav_chunk_t new_chunk;
  bool debug_chunks = debugging_c::requested("wav_reader|wav_reader_chunks");

  m_in->setFilePointer(sizeof(mtx::w64::header_t));

  mxdebug_if(debug_chunks, fmt::format("scan_chunks_wave64() Starting search at file position {0}\n", m_in->getFilePointer()));

  try {
    while (true) {
      new_chunk.pos = m_in->getFilePointer() + sizeof(mtx::w64::chunk_t);
      new_chunk.id  = m_in->read(16);
      new_chunk.len = m_in->read_uint64_le();

      if (!new_chunk.id || (new_chunk.len < sizeof(mtx::w64::chunk_t)))
        return;

      new_chunk.len -= sizeof(mtx::w64::chunk_t);

      mxdebug_if(debug_chunks, fmt::format("scan_chunks_wave64() new chunk at {0} type {1} length {2}\n",
                                           new_chunk.pos - sizeof(mtx::w64::chunk_t), mtx::string::get_displayable(reinterpret_cast<char *>(new_chunk.id->get_buffer()), 4), new_chunk.len + sizeof(mtx::w64::chunk_t)));

      if (!strncasecmp(reinterpret_cast<char *>(new_chunk.id->get_buffer()), "data", 4))
        m_bytes_in_data_chunks += new_chunk.len;

      m_chunks.push_back(new_chunk);
      m_in->setFilePointer(new_chunk.len, libebml::seek_current);

    }
  } catch (...) {
  }
}

std::optional<std::size_t>
wav_reader_c::find_chunk(const char *id,
                         int start_idx,
                         bool allow_empty) {
  for (int idx = start_idx, end = m_chunks.size(); idx < end; ++idx)
    if (!strncasecmp(reinterpret_cast<char *>(m_chunks[idx].id->get_buffer()), id, 4) && (allow_empty || m_chunks[idx].len))
      return idx;

  return {};
}

void
wav_reader_c::identify() {
  if (!m_demuxer) {
    uint16_t format_tag = get_uint16_le(&m_wheader.common.wFormatTag);
    id_result_container_unsupported(m_in->get_file_name(), fmt::format("RIFF WAVE (wFormatTag = 0x{0:04x})", format_tag));
    return;
  }

  auto info = mtx::id::info_c{};
  info.add(mtx::id::audio_channels,           m_demuxer->get_channels());
  info.add(mtx::id::audio_sampling_frequency, m_demuxer->get_sampling_frequency());
  info.add(mtx::id::audio_bits_per_sample,    m_demuxer->get_bits_per_sample());

  id_result_container();
  id_result_track(0, ID_RESULT_TRACK_AUDIO, m_demuxer->m_codec.get_name(), info.get());
}
