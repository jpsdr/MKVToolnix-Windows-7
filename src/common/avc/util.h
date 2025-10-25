/** MPEG-4 p10 video helper functions

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   \file

   \author Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/avc/types.h"

namespace mtx::avc {

memory_cptr parse_sps(memory_cptr const &buffer, sps_info_t &sps, bool keep_ar_info = false, bool fix_bitstream_frame_rate = false, int64_t duration = -1);
bool parse_pps(memory_cptr const &buffer, pps_info_t &pps);

par_extraction_t extract_par(memory_cptr const &buffer);
memory_cptr fix_sps_fps(memory_cptr const &buffer, int64_t duration);
bool is_avc_fourcc(const char *fourcc);
memory_cptr avcc_to_nalus(const uint8_t *buffer, size_t size);

}
