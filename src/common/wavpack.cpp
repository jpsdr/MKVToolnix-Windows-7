/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   helper functions for WAVPACK data

   Written by Steve Lhomme <steve.lhomme@free.fr>.
   Based on a software from David Bryant <dbryant@impulse.net>.
   Modified by Moritz Bunkus <mo@bunkus.online>.
*/

#include <cctype>

#include "common/common_pch.h"
#include "common/debugging.h"
#include "common/endian.h"
#include "common/wavpack.h"

namespace mtx::wavpack {

constexpr auto ID_DSD_BLOCK      = 0x0e;
constexpr auto ID_OPTIONAL_DATA  = 0x20;
constexpr auto ID_UNIQUE         = 0x3f;
constexpr auto ID_ODD_SIZE       = 0x40;
constexpr auto ID_LARGE          = 0x80;

constexpr auto ID_BLOCK_CHECKSUM = (ID_OPTIONAL_DATA | 0xf);
constexpr auto ID_SAMPLE_RATE    = (ID_OPTIONAL_DATA | 0x7);

namespace {
debugging_option_c s_debug{"wavpack"};

constexpr uint32_t s_sample_rates [] = {
   6000,  8000,  9600, 11025, 12000, 16000, 22050,
  24000, 32000, 44100, 48000, 64000, 88200, 96000, 192000
};

} // anonymous namespace

meta_t::meta_t()
  : channel_count(0)
  , bits_per_sample(0)
  , sample_rate(0)
  , samples_per_block(0)
  , has_correction(false)
{
}

static int32_t
read_next_header(mm_io_c &in,
                 header_t *wphdr) {
  char buffer[sizeof(*wphdr)], *sp = buffer + sizeof(*wphdr), *ep = sp;
  uint32_t bytes_skipped = 0;
  int bleft;

  while (true) {
    if (sp < ep) {
      bleft = ep - sp;
      memmove(buffer, sp, bleft);
    } else
      bleft = 0;

    if (in.read(buffer + bleft, sizeof(*wphdr) - bleft) != static_cast<unsigned int>(sizeof(*wphdr) - bleft))
      return -1;

    sp = buffer;

    if ((*sp++ == 'w') && (*sp == 'v') && (*++sp == 'p') && (*++sp == 'k') &&
        !(*++sp & 1) && (sp[2] < 16) && !sp[3] && (sp[5] == 4)) {
      memcpy(wphdr, buffer, sizeof(*wphdr));
      return bytes_skipped;
    }

    while ((sp < ep) && (*sp != 'w'))
      sp++;

    if ((bytes_skipped += sp - buffer) > 1024 * 1024)
      return -1;
  }
}

// Given a WavPack block (complete, but not including the 32-byte header), parse the metadata
// blocks to the end and return the number of bytes used by the trailing block checksum if one
// is found, otherwise return zero. This is the number of bytes that can be deleted from the
// end of the block to erase the checksum. Also, the HAS_CHECKSUM bit in the header flags
// must be reset.

int
checksum_byte_count(uint8_t const *buffer,
                    int bcount) {
  while (bcount >= 2) {
    auto meta_id = *buffer++;
    auto c1      = *buffer++;

    int meta_bc  = c1 << 1;
    bcount      -= 2;

    if (meta_id & ID_LARGE) {
      if (bcount < 2)
        return 0;

      c1       = *buffer++;
      auto c2  = *buffer++;
      meta_bc += (static_cast<uint32_t>(c1) << 9) + (static_cast<uint32_t>(c2) << 17);
      bcount  -= 2;
    }

    if (bcount < meta_bc)
      return 0;

    // if we got a block-checksum and we're at the end of the block, return its size

    if ((meta_id == ID_BLOCK_CHECKSUM) && (meta_bc == bcount))
      return meta_bc + 2;

    bcount -= meta_bc;
    buffer += meta_bc;
  }

  return 0;
}

// Given a WavPack block (complete, but not including the 32-byte header), parse the metadata
// blocks until an ID_SAMPLE_RATE block is found and return the non-standard sample rate
// contained there, or zero if no such block is found.

static int
get_non_standard_rate(uint8_t const *buffer,
                      int bcount) {
  while (bcount >= 2) {
    auto meta_id  = *buffer++;
    auto c1       = *buffer++;

    auto meta_bc  = c1 << 1;
    bcount       -= 2;

    if (meta_id & ID_LARGE) {
      if (bcount < 2)
        return 0;

      c1       = *buffer++;
      auto c2  = *buffer++;
      meta_bc += (static_cast<uint32_t>(c1) << 9) + (static_cast<uint32_t>(c2) << 17);
      bcount  -= 2;
    }

    if (bcount < meta_bc)
      return 0;

    // if we got a sample rate, return it

    if (((meta_id & ID_UNIQUE) == ID_SAMPLE_RATE) && (meta_bc == 4)) {
      int sample_rate = get_uint24_le(buffer);

      // only use 4th byte if it's really there (i.e., size is even)

      if (!(meta_id & ID_ODD_SIZE))
        sample_rate |= static_cast<int32_t>(buffer[3] & 0x7f) << 24;

      return sample_rate;
    }

    bcount -= meta_bc;
    buffer += meta_bc;
  }

  return 0;
}

// Given a WavPack block (complete, but not including the 32-byte header), parse the metadata
// blocks until a DSD audio data block is found and return the sample-rate shift value
// contained there, or zero if no such block is found. The nominal sample rate of DSD audio
// files (found in the header) must be left-shifted by this amount to get the actual "byte"
// sample rate. Note that 8-bit bytes are the "atoms" of the DSD audio coding (for decoding,
// seeking, etc), so the shifted rate must be further multiplied by 8 to get the actual DSD
// bit sample rate.

static int
get_dsd_rate_shifter(uint8_t const *buffer,
                     int bcount) {
  while (bcount >= 2) {
    auto meta_id  = *buffer++;
    auto c1       = *buffer++;

    auto meta_bc  = c1 << 1;
    bcount       -= 2;

    if (meta_id & ID_LARGE) {
      if (bcount < 2)
        return 0;

      c1       = *buffer++;
      auto c2  = *buffer++;
      meta_bc += (static_cast<uint32_t>(c1) << 9) + (static_cast<uint32_t>(c2) << 17);
      bcount  -= 2;
    }

    if (bcount < meta_bc)
      return 0;

    // if we got DSD block, return the specified rate shift amount

    if (((meta_id & ID_UNIQUE) == ID_DSD_BLOCK) && meta_bc && (*buffer <= 31))
      return *buffer;

    bcount -= meta_bc;
    buffer += meta_bc;
  }

  return 0;
}

int32_t
parse_frame(mm_io_c &in,
            header_t &wphdr,
            meta_t &meta,
            bool read_blocked_frames,
            bool keep_initial_position) {
  uint32_t bcount, ck_size{};
  uint64_t first_data_pos = in.getFilePointer();
  bool can_leave = !read_blocked_frames;

  do {
    // read next WavPack header
    bcount = read_next_header(in, &wphdr);

    if (bcount == (uint32_t) -1) {
      return -1;
    }

    // if there's audio samples in there...
    auto block_samples = get_uint32_le(&wphdr.block_samples);
    ck_size            = get_uint32_le(&wphdr.ck_size);

    if (block_samples) {
      auto flags          = get_uint32_le(&wphdr.flags);
      meta.channel_count += (flags & MONO_FLAG) ? 1 : 2;
      if (flags & INITIAL_BLOCK)  {
        int non_standard_rate = 0, dsd_rate_shifter = 0;

        meta.sample_rate = (flags & SRATE_MASK) >> SRATE_LSB;

        // For non-standard sample rates or DSD audio files, we must read and parse the block
        // to actually determine the sample rate. Note that for correction files this will
        // silently fail, but that doesn't seem to cause trouble. An optimization would be to
        // do this only on the very first block of the file since it can't change after that.

        if (meta.sample_rate == 15 || flags & DSD_FLAG) {
          auto adjusted_block_size = ck_size - sizeof(header_t) + 8;
          auto buffer              = memory_c::alloc(adjusted_block_size);

          if (in.read(buffer, adjusted_block_size) != static_cast<unsigned int>(adjusted_block_size))
            return -1;

          if (meta.sample_rate == 15)
            non_standard_rate = get_non_standard_rate(buffer->get_buffer(), adjusted_block_size);

          if (flags & DSD_FLAG)
            dsd_rate_shifter = get_dsd_rate_shifter(buffer->get_buffer(), adjusted_block_size);

          in.skip(-adjusted_block_size);
        }

        if (meta.sample_rate < 15)
          meta.sample_rate = s_sample_rates[meta.sample_rate];
        else if (non_standard_rate)
          meta.sample_rate = non_standard_rate;

        if (flags & DSD_FLAG)
          meta.sample_rate <<= dsd_rate_shifter;

        if (flags & INT32_DATA || flags & FLOAT_DATA)
          meta.bits_per_sample = 32;
        else
          meta.bits_per_sample = ((flags & BYTES_STORED) + 1) << 3;

        meta.samples_per_block = block_samples;

        first_data_pos = in.getFilePointer();
        meta.channel_count = (flags & MONO_FLAG) ? 1 : 2;
        if (flags & FINAL_BLOCK) {
          can_leave = true;
          mxdebug_if(s_debug, fmt::format("reader: {0} block: {1}, {2} bytes\n", flags & MONO_FLAG   ? "mono"   : "stereo", flags & HYBRID_FLAG ? "hybrid" : "lossless", ck_size + 8));
        }
      } else {
        if (flags & FINAL_BLOCK) {
          can_leave = true;
          mxdebug_if(s_debug, fmt::format("reader: {0} chans, mode: {1}, {2} bytes\n", meta.channel_count, flags & HYBRID_FLAG ? "hybrid" : "lossless", ck_size + 8));
        }
      }
    } else
      mxwarn(Y("reader: non-audio block found\n"));
    if (!can_leave) {
      in.skip(ck_size - sizeof(header_t) + 8);
    }
  } while (!can_leave);

  if (keep_initial_position)
    in.setFilePointer(first_data_pos);

  return ck_size - sizeof(header_t) + 8;
}

} // namespace mtx::wavpack
