/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   DTS demultiplexer module

   Written by Peter Niemayer <niemayer@isg.de>.
   Modified by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#if defined(HAVE_UNISTD_H)
# include <unistd.h>
#endif  // HAVE_UNISTD_H

#include "common/codec.h"
#include "common/debugging.h"
#include "common/dts.h"
#include "common/error.h"
#include "common/id_info.h"
#include "common/mm_io_x.h"
#include "input/r_dts.h"
#include "merge/input_x.h"
#include "output/p_dts.h"

#define READ_SIZE 16384

int
dts_reader_c::probe_file(mm_io_c &in,
                         uint64_t size,
                         bool strict_mode) {
  if (size < READ_SIZE)
    return 0;

  try {
    auto chunks         = scan_chunks(in);
    auto strmdata_chunk = brng::find_if(chunks, [](chunk_t const &chunk) { return chunk.type == chunk_type_e::strmdata; });
    if (strmdata_chunk == chunks.end())
      return 0;

    unsigned char buf[READ_SIZE];
    auto bytes_to_read = std::min<uint64_t>(strmdata_chunk->data_size, READ_SIZE);

    in.setFilePointer(strmdata_chunk->data_start, seek_beginning);
    if (in.read(buf, bytes_to_read) != bytes_to_read)
      return 0;

    bool convert_14_to_16 = false, swap_bytes = false;
    if (!strict_mode && mtx::dts::detect(buf, READ_SIZE, convert_14_to_16, swap_bytes))
      return 1;

    int pos = mtx::dts::find_consecutive_headers(buf, READ_SIZE, 5);
    if ((!strict_mode && (0 <= pos)) || (strict_mode && (0 == pos)))
      return 1;

  } catch (...) {
  }

  return 0;
}

dts_reader_c::dts_reader_c(const track_info_c &ti,
                           const mm_io_cptr &in)
  : generic_reader_c(ti, in)
  , m_af_buf(memory_c::alloc(2 * READ_SIZE))
  , m_cur_buf(0)
  , m_dts14_to_16(false)
  , m_swap_bytes(false)
  , m_debug{"dts|dts_reader"}
  , m_codec{codec_c::look_up(codec_c::type_e::A_DTS)}
  , m_chunks{scan_chunks(*in)}
  , m_current_chunk{brng::find_if(m_chunks, [](chunk_t const &chunk) { return chunk.type == chunk_type_e::strmdata; })}
{
  m_buf[0] = reinterpret_cast<unsigned short *>(m_af_buf->get_buffer());
  m_buf[1] = reinterpret_cast<unsigned short *>(m_af_buf->get_buffer() + READ_SIZE);
}

void
dts_reader_c::read_headers() {
  try {
    m_in->setFilePointer(m_current_chunk->data_start);
    auto bytes_to_read = std::min<int64_t>(m_current_chunk->data_size, READ_SIZE);
    if (m_in->read(m_buf[m_cur_buf], bytes_to_read) != bytes_to_read)
      throw mtx::input::header_parsing_x();
    m_in->setFilePointer(m_current_chunk->data_start);

  } catch (...) {
    throw mtx::input::open_x();
  }

  mtx::dts::detect(m_buf[m_cur_buf], READ_SIZE, m_dts14_to_16, m_swap_bytes);

  mxdebug_if(m_debug, boost::format("DTS: 14->16 %1% swap %2%\n") % m_dts14_to_16 % m_swap_bytes);

  decode_buffer(READ_SIZE);
  int pos = mtx::dts::find_header(reinterpret_cast<const unsigned char *>(m_buf[m_cur_buf]), READ_SIZE, m_dtsheader);

  if (0 > pos)
    throw mtx::input::header_parsing_x();

  m_ti.m_id = 0;          // ID for this track.

  m_codec.set_specialization(m_dtsheader.get_codec_specialization());

  show_demuxer_info();
}

dts_reader_c::~dts_reader_c() {
}

int
dts_reader_c::decode_buffer(size_t length) {
  if (m_swap_bytes) {
    swab(reinterpret_cast<char *>(m_buf[m_cur_buf]), reinterpret_cast<char *>(m_buf[m_cur_buf ^ 1]), length);
    m_cur_buf ^= 1;
  }

  if (m_dts14_to_16) {
    mtx::dts::convert_14_to_16_bits(m_buf[m_cur_buf], length / 2, m_buf[m_cur_buf ^ 1]);
    m_cur_buf ^= 1;
    length     = length * 7 / 8;
  }

  return length;
}

void
dts_reader_c::create_packetizer(int64_t) {
  if (!demuxing_requested('a', 0) || (NPTZR() != 0))
    return;

  add_packetizer(new dts_packetizer_c(this, m_ti, m_dtsheader));
  show_packetizer_info(0, PTZR0);

  if (m_debug)
    m_dtsheader.print();
}

file_status_e
dts_reader_c::read(generic_packetizer_c *,
                   bool) {
  auto chunks_end = m_chunks.end();

  if (m_current_chunk == chunks_end)
    return flush_packetizers();

  auto bytes_to_read = std::min<int64_t>(m_current_chunk->data_end - std::min(m_in->getFilePointer(), m_current_chunk->data_end), READ_SIZE);
  if (m_dts14_to_16)
    bytes_to_read &= ~0xf;

  auto num_read = m_in->read(m_buf[m_cur_buf], bytes_to_read);

  if (0 >= num_read)
    return flush_packetizers();

  int num_to_output = decode_buffer(num_read);

  PTZR0->process(new packet_t(new memory_c(m_buf[m_cur_buf], num_to_output, false)));

  if (m_in->eof() || (num_read < bytes_to_read))
    return flush_packetizers();

  if (m_in->getFilePointer() < m_current_chunk->data_end)
    return FILE_STATUS_MOREDATA;

  ++m_current_chunk;

  while ((m_current_chunk != chunks_end) && (m_current_chunk->type != chunk_type_e::strmdata))
    ++m_current_chunk;

  if (m_current_chunk == chunks_end)
    return flush_packetizers();

  m_in->setFilePointer(m_current_chunk->data_start);
  return FILE_STATUS_MOREDATA;
}

void
dts_reader_c::identify() {
  auto info = mtx::id::info_c{};
  info.add(mtx::id::audio_channels,           m_dtsheader.get_total_num_audio_channels());
  info.add(mtx::id::audio_sampling_frequency, m_dtsheader.get_effective_sampling_frequency());
  info.add(mtx::id::audio_bits_per_sample,    std::max(m_dtsheader.source_pcm_resolution, 0));

  id_result_container();
  id_result_track(0, ID_RESULT_TRACK_AUDIO, m_codec.get_name(), info.get());
}

dts_reader_c::chunks_t
dts_reader_c::scan_chunks(mm_io_c &in) {
  static auto s_debug = debugging_option_c{"dts_reader|dts_reader_chunks"};
  auto chunks         = chunks_t{};

  try {
    auto const file_size = static_cast<uint64_t>(in.get_size());

    in.setFilePointer(0, seek_beginning);
    auto type = static_cast<chunk_type_e>(in.read_uint64_be());

    if (type != chunk_type_e::dtshdhdr) {
      chunks.emplace_back(chunk_type_e::strmdata, 0ull, file_size);
      return chunks;
    }

    auto next_chunk_start = 0ull;

    while (next_chunk_start < (file_size - 16)) {
      in.setFilePointer(next_chunk_start);
      type           = static_cast<chunk_type_e>(in.read_uint64_be());
      auto data_size = std::min<uint64_t>(in.read_uint64_be(), file_size - next_chunk_start - 16);

      chunks.emplace_back(type, next_chunk_start + 16, data_size);
      next_chunk_start = chunks.back().data_end;
    }

  } catch (mtx::mm_io::exception &) {
  }

  if (s_debug)
    for (auto const &chunk : chunks)
      mxinfo(boost::format("DTS chunk type %|1$16x| at %2% data size %3% data end %4%\n") % static_cast<uint64_t>(chunk.type) % (chunk.data_start - 16) % chunk.data_size % chunk.data_end);

  return chunks;
}
