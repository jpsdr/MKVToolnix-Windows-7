/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   definitions for the TTA file format

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx::tta {

/* All integers are little endian. */

constexpr auto FRAME_TIME = 1.04489795918367346939;

#if defined(COMP_MSC)
#pragma pack(push,1)
#endif
struct PACKED_STRUCTURE file_header_t {
  char signature[4];            /* TTA1 */
  uint16_t audio_format;        /* 1 for 32 bits per sample, 3 otherwise? */
  uint16_t channels;
  uint16_t bits_per_sample;
  uint32_t sample_rate;
  uint32_t data_length;
  uint32_t crc;
};
#if defined(COMP_MSC)
#pragma pack(pop)
#endif

} // namespace mtx::tta
