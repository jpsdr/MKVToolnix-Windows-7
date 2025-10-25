/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   WAV DTS demuxer module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

constexpr auto DTS_READ_SIZE = 65536;

class wav_dts_demuxer_c: public wav_demuxer_c {
private:
  mtx::dts::parser_c m_parser;
  memory_cptr m_read_buffer;

public:
  wav_dts_demuxer_c(wav_reader_c *reader, wave_header *wheader);

  virtual ~wav_dts_demuxer_c();

  virtual bool probe(mm_io_cptr &io);

  virtual int64_t get_preferred_input_size() {
    return DTS_READ_SIZE;
  };

  virtual uint8_t *get_buffer() {
    return m_read_buffer->get_buffer();
  }

  virtual void process(int64_t len);
  virtual generic_packetizer_c *create_packetizer();

  virtual unsigned int get_channels() const;
  virtual unsigned int get_sampling_frequency() const;
  virtual unsigned int get_bits_per_sample() const;
};
