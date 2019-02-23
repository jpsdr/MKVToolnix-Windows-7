/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   helper functions for WAVPACK data

   Written by Steve Lhomme <steve.lhomme@free.fr>.
   Based on a software from David Bryant <dbryant@impulse.net>.
   Modified by Moritz Bunkus <moritz@bunkus.org>.
*/

#include <cctype>

#include "common/common_pch.h"
#include "common/endian.h"
#include "common/wavpack.h"

const uint32_t sample_rates [] = {
   6000,  8000,  9600, 11025, 12000, 16000, 22050,
  24000, 32000, 44100, 48000, 64000, 88200, 96000, 192000 };

wavpack_meta_t::wavpack_meta_t()
  : channel_count(0)
  , bits_per_sample(0)
  , sample_rate(0)
  , samples_per_block(0)
  , has_correction(false)
{
}

static int32_t
read_next_header(mm_io_c &in,
                 wavpack_header_t *wphdr) {
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

int32_t
wv_parse_frame(mm_io_c &in,
               wavpack_header_t &wphdr,
               wavpack_meta_t &meta,
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
      meta.channel_count += (flags & WV_MONO_FLAG) ? 1 : 2;
      if (flags & WV_INITIAL_BLOCK)  {
        meta.sample_rate = (flags & WV_SRATE_MASK) >> WV_SRATE_LSB;

        if (meta.sample_rate == 15)
          mxwarn(Y("wavpack_reader: unknown sample rate!\n"));
        else
          meta.sample_rate = sample_rates[meta.sample_rate];

        if (flags & WV_INT32_DATA || flags & WV_FLOAT_DATA)
          meta.bits_per_sample = 32;
        else
          meta.bits_per_sample = ((flags & WV_BYTES_STORED) + 1) << 3;

        meta.samples_per_block = block_samples;

        first_data_pos = in.getFilePointer();
        meta.channel_count = (flags & WV_MONO_FLAG) ? 1 : 2;
        if (flags & WV_FINAL_BLOCK) {
          can_leave = true;
          mxverb(3,
                 fmt::format("wavpack_reader: {0} block: {1}, {2} bytes\n",
                             flags & WV_MONO_FLAG   ? "mono"   : "stereo",
                             flags & WV_HYBRID_FLAG ? "hybrid" : "lossless",
                             ck_size + 8));
        }
      } else {
        if (flags & WV_FINAL_BLOCK) {
          can_leave = true;
          mxverb(2,
                 fmt::format("wavpack_reader: {0} chans, mode: {1}, {2} bytes\n",
                             meta.channel_count,
                             flags & WV_HYBRID_FLAG ? "hybrid" : "lossless",
                             ck_size + 8));
        }
      }
    } else
      mxwarn(Y("wavpack_reader: non-audio block found\n"));
    if (!can_leave) {
      in.skip(ck_size - sizeof(wavpack_header_t) + 8);
    }
  } while (!can_leave);

  if (keep_initial_position)
    in.setFilePointer(first_data_pos);

  return ck_size - sizeof(wavpack_header_t) + 8;
}
