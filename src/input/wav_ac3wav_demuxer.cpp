/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   WAV AC-3 (WAV mode) demuxer module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "avilib.h"
#include "common/ac3.h"
#include "common/bswap.h"
#include "common/endian.h"
#include "input/r_wav.h"
#include "input/wav_ac3acm_demuxer.h"
#include "input/wav_ac3wav_demuxer.h"
#include "output/p_ac3.h"

wav_ac3wav_demuxer_c::wav_ac3wav_demuxer_c(wav_reader_c *reader,
                                           wave_header  *wheader)
  : wav_ac3acm_demuxer_c{reader, wheader}
{
}

wav_ac3wav_demuxer_c::~wav_ac3wav_demuxer_c() {
}

bool
wav_ac3wav_demuxer_c::probe(mm_io_cptr &io) {
  io->save_pos();
  int len = io->read(m_buf[m_cur_buf]->get_buffer(), AC3WAV_BLOCK_SIZE);
  io->restore_pos();

  if (decode_buffer(len) > 0)
    return true;

  m_swap_bytes = true;
  return decode_buffer(len) > 0;
}

int
wav_ac3wav_demuxer_c::decode_buffer(int len) {
  if (len < 8)
    return -1;

  if (m_swap_bytes) {
    memcpy(                 m_buf[m_cur_buf ^ 1]->get_buffer(), m_buf[m_cur_buf]->get_buffer(),         8);
    mtx::bytes::swap_buffer(m_buf[m_cur_buf]->get_buffer() + 8, m_buf[m_cur_buf ^ 1]->get_buffer() + 8, len - 8, 2);
    m_cur_buf ^= 1;
  }

  auto base = m_buf[m_cur_buf]->get_buffer();

  if ((get_uint16_le(&base[0]) != AC3WAV_SYNC_WORD1) || (get_uint16_le(&base[2]) != AC3WAV_SYNC_WORD2) || (0x01 != base[4]))
    return -1;

  int payload_len = get_uint16_le(&base[6]) / 8;

  if ((payload_len + 8) > len)
    return -1;

  if (!m_ac3header.decode_header(&base[8], payload_len))
    return -1;

  m_codec = m_ac3header.get_codec();

  return payload_len;
}

void
wav_ac3wav_demuxer_c::process(int64_t size) {
  if (0 >= size)
    return;

  long dec_len = decode_buffer(size);
  if (0 < dec_len)
    m_ptzr->process(std::make_shared<packet_t>(memory_c::borrow(m_buf[m_cur_buf]->get_buffer() + 8, dec_len)));
}

unsigned int
wav_ac3wav_demuxer_c::get_channels()
  const {
  return m_ac3header.m_channels;
}

unsigned int
wav_ac3wav_demuxer_c::get_sampling_frequency()
  const {
  return m_ac3header.m_sample_rate;
}

unsigned int
wav_ac3wav_demuxer_c::get_bits_per_sample()
  const {
  return 0;
}
