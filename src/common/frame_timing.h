/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/math.h"

namespace mtx { namespace frame_timing {

struct common_frame_rate_t {
  int64_t duration;
  int64_rational_c frame_rate;
};

extern std::vector<common_frame_rate_t> g_common_frame_rates;

int64_rational_c determine_frame_rate(int64_t duration, int64_t max_difference = 20'000);

}}
