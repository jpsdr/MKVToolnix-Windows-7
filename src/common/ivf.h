/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   definitions and helper functions for IVF data

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/codec.h"

/* All integers are little endian. */

namespace ivf {

#if defined(COMP_MSC)
#pragma pack(push,1)
#endif
struct PACKED_STRUCTURE file_header_t {
  uint8_t  file_magic[4]; // "DKIF"
  uint16_t version;
  uint16_t header_size;
  uint8_t  fourcc[4];     // "VP80"
  uint16_t width;
  uint16_t height;
  uint32_t frame_rate_num;
  uint32_t frame_rate_den;
  uint32_t frame_count;
  uint32_t unused;

  file_header_t();
  codec_c get_codec() const;
};

struct PACKED_STRUCTURE frame_header_t {
  uint32_t frame_size;
  uint64_t timestamp;

  frame_header_t();
};

bool is_keyframe(const memory_cptr &buffer, codec_c::type_e codec);

#if defined(COMP_MSC)
#pragma pack(pop)
#endif
}
