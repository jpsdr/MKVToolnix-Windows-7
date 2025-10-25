/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   WAV AC-3 (ACM mode) demuxer module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "avilib.h"
#include "common/ac3.h"
#include "common/bswap.h"
#include "input/r_wav.h"
#include "input/wav_ac3acm_demuxer.h"
#include "output/p_ac3.h"

wav_ac3acm_demuxer_c::wav_ac3acm_demuxer_c(wav_reader_c *reader,
                                           wave_header  *wheader)
  : wav_demuxer_c{reader, wheader}
  , m_buf{memory_c::alloc(AC3ACM_READ_SIZE), memory_c::alloc(AC3ACM_READ_SIZE)}
  , m_cur_buf{}
  , m_swap_bytes{}
{
  m_codec  = codec_c::look_up(codec_c::type_e::A_AC3);
}

wav_ac3acm_demuxer_c::~wav_ac3acm_demuxer_c() {
}

bool
wav_ac3acm_demuxer_c::probe(mm_io_cptr &io) {
  io->save_pos();
  int len = io->read(m_buf[m_cur_buf]->get_buffer(), AC3ACM_READ_SIZE);
  io->restore_pos();

  mtx::ac3::parser_c parser;
  int pos = parser.find_consecutive_frames(m_buf[m_cur_buf]->get_buffer(), len, 4);

  if (-1 == pos) {
    m_swap_bytes = true;
    decode_buffer(len);
    pos = parser.find_consecutive_frames(m_buf[m_cur_buf]->get_buffer(), len, 4);
  }

  if (-1 == pos)
    return false;

  if (!m_ac3header.decode_header(m_buf[m_cur_buf]->get_buffer() + pos, len - pos))
    return false;

  m_codec = m_ac3header.get_codec();

  return true;
}

int
wav_ac3acm_demuxer_c::decode_buffer(int len) {
  if ((2 < len) && m_swap_bytes) {
    mtx::bytes::swap_buffer(m_buf[m_cur_buf]->get_buffer(), m_buf[m_cur_buf ^ 1]->get_buffer(), len, 2);
    m_cur_buf ^= 1;
  }

  return 0;
}

generic_packetizer_c *
wav_ac3acm_demuxer_c::create_packetizer() {
  m_ptzr = new ac3_packetizer_c(m_reader, m_ti, m_ac3header.m_sample_rate, m_ac3header.m_channels, m_ac3header.m_bs_id);

  m_reader->show_packetizer_info(0, *m_ptzr);

  return m_ptzr;
}

void
wav_ac3acm_demuxer_c::process(int64_t size) {
  if (0 >= size)
    return;

  decode_buffer(size);
  m_ptzr->process(std::make_shared<packet_t>(memory_c::borrow(m_buf[m_cur_buf]->get_buffer(), size)));
}

unsigned int
wav_ac3acm_demuxer_c::get_channels()
  const {
  return m_ac3header.m_channels;
}

unsigned int
wav_ac3acm_demuxer_c::get_sampling_frequency()
  const {
  return m_ac3header.m_sample_rate;
}

unsigned int
wav_ac3acm_demuxer_c::get_bits_per_sample()
  const {
  return 0;
}
