/** MPEG-4 p10 video helper functions

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   \file

   \author Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/avc_types.h"

namespace mtx { namespace avc {

memory_cptr parse_sps(memory_cptr const &buffer, sps_info_t &sps, bool keep_ar_info = false, bool fix_bitstream_frame_rate = false, int64_t duration = -1);
bool parse_pps(memory_cptr const &buffer, pps_info_t &pps);

par_extraction_t extract_par(memory_cptr const &buffer);
memory_cptr fix_sps_fps(memory_cptr const &buffer, int64_t duration);
bool is_avc_fourcc(const char *fourcc);
memory_cptr avcc_to_nalus(const unsigned char *buffer, size_t size);

}}
