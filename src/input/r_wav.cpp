/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   WAV reader module

   Written by Moritz Bunkus <moritz@bunkus.org>.
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
#include "common/strings/formatting.h"
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

int
wav_reader_c::probe_file(mm_io_c *in,
                         uint64_t size) {
  wave_header wheader;

  if (sizeof(wave_header) > size)
    return 0;
  try {
    in->setFilePointer(0, seek_beginning);
    if (in->read(&wheader.riff, sizeof(wheader.riff)) != sizeof(wheader.riff))
      return 0;
    in->setFilePointer(0, seek_beginning);
  } catch (...) {
    return 0;
  }

  if (strncmp((char *)wheader.riff.id,      "RIFF", 4) ||
      strncmp((char *)wheader.riff.wave_id, "WAVE", 4))
    return 0;

  return 1;
}

wav_reader_c::wav_reader_c(const track_info_c &ti,
                           const mm_io_cptr &in)
  : generic_reader_c(ti, in)
  , m_bytes_in_data_chunks(0)
  , m_remaining_bytes_in_current_data_chunk(0)
  , m_cur_data_chunk_idx(0)
{
}

void
wav_reader_c::read_headers() {
  if (!wav_reader_c::probe_file(m_in.get(), m_size))
    throw mtx::input::invalid_format_x();

  parse_file();
  create_demuxer();
}

wav_reader_c::~wav_reader_c() {
}

void
wav_reader_c::parse_file() {
  int chunk_idx;

  if (m_in->read(&m_wheader.riff, sizeof(m_wheader.riff)) != sizeof(m_wheader.riff))
    throw mtx::input::header_parsing_x();

  scan_chunks();

  if ((chunk_idx = find_chunk("fmt ")) == -1)
    throw mtx::input::header_parsing_x();

  m_in->setFilePointer(m_chunks[chunk_idx].pos, seek_beginning);

  try {
    if (m_in->read(&m_wheader.format, sizeof(m_wheader.format)) != sizeof(m_wheader.format))
      throw false;

    if (static_cast<uint64_t>(m_chunks[chunk_idx].len) >= sizeof(alWAVEFORMATEXTENSIBLE)) {
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

  if ((m_cur_data_chunk_idx = find_chunk("data", 0, false)) == -1)
    throw mtx::input::header_parsing_x();

  if (debugging_c::requested("wav_reader|wav_reader_headers"))
    dump_headers();

  m_in->setFilePointer(m_chunks[m_cur_data_chunk_idx].pos + sizeof(struct chunk_struct), seek_beginning);

  m_remaining_bytes_in_current_data_chunk = m_chunks[m_cur_data_chunk_idx].len;
}

void
wav_reader_c::dump_headers() {
  mxinfo(boost::format("File '%1%' wave_header dump\n"
                       "  riff:\n"
                       "    id:      %2%%3%%4%%5%\n"
                       "    len:     %6%\n"
                       "    wave_id: %7%%8%%9%%10%\n"
                       "  common:\n"
                       "    wFormatTag:       %|11$04x|\n"
                       "    wChannels:        %12%\n"
                       "    dwSamplesPerSec:  %13%\n"
                       "    dwAvgBytesPerSec: %14%\n"
                       "    wBlockAlign:      %15%\n"
                       "    wBitsPerSample:   %16%\n"
                       "  actual format_tag:  %17%\n")
         % m_ti.m_fname
         % char(m_wheader.riff.id[0]) % char(m_wheader.riff.id[1]) % char(m_wheader.riff.id[2]) % char(m_wheader.riff.id[3])
         % get_uint32_le(&m_wheader.riff.len)
         % char(m_wheader.riff.wave_id[0]) % char(m_wheader.riff.wave_id[1]) % char(m_wheader.riff.wave_id[2]) % char(m_wheader.riff.wave_id[3])
         % get_uint16_le(&m_wheader.common.wFormatTag)
         % get_uint16_le(&m_wheader.common.wChannels)
         % get_uint32_le(&m_wheader.common.dwSamplesPerSec)
         % get_uint32_le(&m_wheader.common.dwAvgBytesPerSec)
         % get_uint16_le(&m_wheader.common.wBlockAlign)
         % get_uint16_le(&m_wheader.common.wBitsPerSample)
         % m_format_tag);
}

void
wav_reader_c::create_demuxer() {
  m_ti.m_id = 0;                    // ID for this track.

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
  if (!demuxing_requested('a', 0) || (NPTZR() != 0) || !m_demuxer)
    return;

  add_packetizer(m_demuxer->create_packetizer());
}

file_status_e
wav_reader_c::read(generic_packetizer_c *,
                   bool) {
  if (!m_demuxer)
    return FILE_STATUS_DONE;

  int64_t        requested_bytes = std::min(m_remaining_bytes_in_current_data_chunk, m_demuxer->get_preferred_input_size());
  unsigned char *buffer          = m_demuxer->get_buffer();
  int64_t        num_read;

  num_read = m_in->read(buffer, requested_bytes);

  if (0 >= num_read)
    return flush_packetizers();

  m_demuxer->process(num_read);

  m_remaining_bytes_in_current_data_chunk -= num_read;

  if (!m_remaining_bytes_in_current_data_chunk) {
    m_cur_data_chunk_idx = find_chunk("data", m_cur_data_chunk_idx + 1, false);

    if (-1 == m_cur_data_chunk_idx)
      return flush_packetizers();

    m_in->setFilePointer(m_chunks[m_cur_data_chunk_idx].pos + sizeof(struct chunk_struct), seek_beginning);

    m_remaining_bytes_in_current_data_chunk = m_chunks[m_cur_data_chunk_idx].len;
  }

  return FILE_STATUS_MOREDATA;
}

void
wav_reader_c::scan_chunks() {
  wav_chunk_t new_chunk;
  bool debug_chunks = debugging_c::requested("wav_reader|wav_reader_chunks");

  try {
    int64_t file_size = m_in->get_size();

    while (true) {
      new_chunk.pos = m_in->getFilePointer();

      if (m_in->read(new_chunk.id, 4) != 4)
        return;

      new_chunk.len = m_in->read_uint32_le();

      if (debug_chunks)
        mxinfo(boost::format("wav_reader_c::scan_chunks() new chunk at %1% type %2% length %3%\n")
               % new_chunk.pos % get_displayable_string(new_chunk.id, 4) % new_chunk.len);

      if (!strncasecmp(new_chunk.id, "data", 4))
        m_bytes_in_data_chunks += new_chunk.len;

      else if (!m_chunks.empty() && !strncasecmp(m_chunks.back().id, "data", 4) && (file_size > 0x100000000ll)) {
        wav_chunk_t &previous_chunk  = m_chunks.back();
        int64_t this_chunk_len       = file_size - previous_chunk.pos - sizeof(struct chunk_struct);
        m_bytes_in_data_chunks      -= previous_chunk.len;
        m_bytes_in_data_chunks      += this_chunk_len;
        previous_chunk.len           = this_chunk_len;

        if (debug_chunks)
          mxinfo(boost::format("wav_reader_c::scan_chunks() hugh data chunk with wrong length at %1%; re-calculated from file size; new length %2%\n")
                 % previous_chunk.pos % previous_chunk.len);

        break;
      }

      m_chunks.push_back(new_chunk);
      m_in->setFilePointer(new_chunk.len, seek_current);

    }
  } catch (...) {
  }
}

int
wav_reader_c::find_chunk(const char *id,
                         int start_idx,
                         bool allow_empty) {
  size_t idx;

  for (idx = start_idx; idx < m_chunks.size(); ++idx)
    if (!strncasecmp(m_chunks[idx].id, id, 4) && (allow_empty || m_chunks[idx].len))
      return idx;

  return -1;
}

void
wav_reader_c::identify() {
  if (!m_demuxer) {
    uint16_t format_tag = get_uint16_le(&m_wheader.common.wFormatTag);
    id_result_container_unsupported(m_in->get_file_name(), (boost::format("RIFF WAVE (wFormatTag = 0x%|1$04x|)") % format_tag).str());
    return;
  }

  auto info = mtx::id::info_c{};
  info.add(mtx::id::audio_channels,           m_demuxer->get_channels());
  info.add(mtx::id::audio_sampling_frequency, m_demuxer->get_sampling_frequency());
  info.add(mtx::id::audio_bits_per_sample,    m_demuxer->get_bits_per_sample());

  id_result_container();
  id_result_track(0, ID_RESULT_TRACK_AUDIO, m_demuxer->m_codec.get_name(), info.get());
}
