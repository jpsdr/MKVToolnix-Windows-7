/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   definitions for the WAVPACK file format

   Written by Steve Lhomme <steve.lhomme@free.fr>.
   Based on a software from David Bryant <dbryant@impulse.net>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/mm_io.h"

namespace mtx::wavpack {

/* All integers are little endian. */

#if defined(COMP_MSC)
#pragma pack(push,1)
#endif
struct PACKED_STRUCTURE header_t {
  char ck_id [4];         // "wvpk"
  uint32_t ck_size;       // size of entire frame (minus 8, of course)
  uint16_t version;       // maximum of 0x410 as of 2016-07-03
  uint8_t track_no;       // track number (0 if not used, like now)
  uint8_t index_no;       // remember these? (0 if not used, like now)
  uint32_t total_samples; // for entire file (-1 if unknown)
  uint32_t block_index;   // index of first sample in block (to file begin)
  uint32_t block_samples; // # samples in this block
  uint32_t flags;         // various flags for id and decoding
  uint32_t crc;           // crc for actual decoded data
};
#if defined(COMP_MSC)
#pragma pack(pop)
#endif

struct meta_t {
  int channel_count;
  int bits_per_sample;
  uint32_t sample_rate;
  uint32_t samples_per_block;
  bool has_correction;

  meta_t();
};

// or-values for "flags"

constexpr auto BYTES_STORED   = 3;          // 1-4 bytes/sample
constexpr auto MONO_FLAG      = 4;          // not stereo
constexpr auto HYBRID_FLAG    = 8;          // hybrid mode
constexpr auto JOINT_STEREO   = 0x10;       // joint stereo
constexpr auto CROSS_DECORR   = 0x20;       // no-delay cross decorrelation
constexpr auto HYBRID_SHAPE   = 0x40;       // noise shape (hybrid mode only)
constexpr auto FLOAT_DATA     = 0x80;       // ieee 32-bit floating point data

constexpr auto INT32_DATA     = 0x100;      // special extended int handling
constexpr auto HYBRID_BITRATE = 0x200;      // bitrate noise (hybrid mode only)
constexpr auto HYBRID_BALANCE = 0x400;      // balance noise (hybrid stereo mode only)

constexpr auto INITIAL_BLOCK  = 0x800;      // initial block of multichannel segment
constexpr auto FINAL_BLOCK    = 0x1000;     // final block of multichannel segment

constexpr auto SHIFT_LSB      = 13;
constexpr auto SHIFT_MASK     = (0x1fL << SHIFT_LSB);

constexpr auto MAG_LSB        = 18;
constexpr auto MAG_MASK       = (0x1fL << MAG_LSB);

constexpr auto SRATE_LSB      = 23;
constexpr auto SRATE_MASK     = (0xfL << SRATE_LSB);

constexpr auto NEW_SHAPING    = 0x20000000; // use IIR filter for negative shaping

// Introduced in WavPack 5.0:
constexpr auto HAS_CHECKSUM   = 0x10000000; // block contains a trailing checksum
constexpr auto DSD_FLAG       = 0x80000000; // block is encoded DSD (1-bit PCM)

int32_t parse_frame(mm_io_c &mm_io, header_t &header, meta_t &meta, bool read_blocked_frames, bool keep_initial_position);
int checksum_byte_count(uint8_t const *buffer, int bcount);

} // namespace mtx::wavpack
