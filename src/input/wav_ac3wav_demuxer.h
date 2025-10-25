/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   WAV AC-3 (WAV mode) demuxer module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

constexpr auto AC3WAV_BLOCK_SIZE = 6144;
constexpr auto AC3WAV_SYNC_WORD1 = 0xf872;
constexpr auto AC3WAV_SYNC_WORD2 = 0x4e1f;

// Structure of AC-3-in-WAV:
//
// AA BB C D EE F..F 0..0
//
// Index | Size       | Meaning
// ------+------------+---------------------------
// A     | 2          | AC3WAV_SYNC_WORD1
// B     | 2          | AC3WAV_SYNC_WORD2
// C     | 1          | BSMOD
// D     | 1          | data type; 0x01 = AC-3
// E     | 2          | number of bits in payload
// F     | E/8        | one AC-3 packet
// 0     | 6144-E/8-8 | zero padding

class wav_ac3wav_demuxer_c: public wav_ac3acm_demuxer_c {
public:
  wav_ac3wav_demuxer_c(wav_reader_c *n_reader, wave_header *n_wheader);

  virtual ~wav_ac3wav_demuxer_c();

  virtual bool probe(mm_io_cptr &io);

  virtual int64_t get_preferred_input_size() {
    return AC3WAV_BLOCK_SIZE;
  };

  virtual void process(int64_t len);

  virtual unsigned int get_channels() const;
  virtual unsigned int get_sampling_frequency() const;
  virtual unsigned int get_bits_per_sample() const;

protected:
  virtual int decode_buffer(int len);
};
