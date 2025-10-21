/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   WAV DTS demuxer module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "avilib.h"
#include "common/dts.h"
#include "common/dts_parser.h"
#include "input/r_wav.h"
#include "input/wav_dts_demuxer.h"
#include "output/p_dts.h"

wav_dts_demuxer_c::wav_dts_demuxer_c(wav_reader_c *reader,
                                     wave_header  *wheader)
  : wav_demuxer_c{reader, wheader}
  , m_read_buffer{memory_c::alloc(DTS_READ_SIZE)}
{
  m_codec = codec_c::look_up(codec_c::type_e::A_DTS);
}

wav_dts_demuxer_c::~wav_dts_demuxer_c() {
}

bool
wav_dts_demuxer_c::probe(mm_io_cptr &io) {
  io->save_pos();
  auto read_buf = memory_c::alloc(DTS_READ_SIZE);
  read_buf->set_size(io->read(read_buf->get_buffer(), DTS_READ_SIZE));
  io->restore_pos();

  if (!m_parser.detect(*read_buf, 5))
    return false;

  m_codec.set_specialization(m_parser.get_first_header().get_codec_specialization());

  return true;
}

generic_packetizer_c *
wav_dts_demuxer_c::create_packetizer() {
  m_ptzr = new dts_packetizer_c(m_reader, m_ti, m_parser.get_first_header());

  // .wav with DTS are always filled up with other stuff to match the bitrate.
  static_cast<dts_packetizer_c *>(m_ptzr)->set_skipping_is_normal(true);

  m_reader->show_packetizer_info(0, *m_ptzr);

  return m_ptzr;
}

void
wav_dts_demuxer_c::process(int64_t size) {
  if (0 >= size)
    return;

  auto decoded = m_parser.decode(m_read_buffer->get_buffer(), size);
  m_ptzr->process(std::make_shared<packet_t>(decoded));
}

unsigned int
wav_dts_demuxer_c::get_channels()
  const {
  return m_parser.get_first_header().get_total_num_audio_channels();
}

unsigned int
wav_dts_demuxer_c::get_sampling_frequency()
  const {
  return m_parser.get_first_header().get_effective_sampling_frequency();
}

unsigned int
wav_dts_demuxer_c::get_bits_per_sample()
  const {
  return m_parser.get_first_header().source_pcm_resolution;
}
