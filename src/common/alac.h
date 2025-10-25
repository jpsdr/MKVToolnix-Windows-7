/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   helper functions for ALAC data

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx::alac {

#if defined(COMP_MSC)
#pragma pack(push,1)
#endif

struct PACKED_STRUCTURE codec_config_t {
  uint32_t frame_length;
  uint8_t  compatible_version;
  uint8_t  bit_depth;
  uint8_t  rice_history_mult;
  uint8_t  rice_initial_history;
  uint8_t  rice_limit;
  uint8_t  num_channels;
  uint16_t max_run;
  uint32_t max_frame_bytes;
  uint32_t avg_bit_rate;
  uint32_t sample_rate;
};

#if defined(COMP_MSC)
#pragma pack(pop)
#endif

}
