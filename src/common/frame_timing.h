/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/math_fwd.h"

namespace mtx::frame_timing {

struct common_frame_rate_t {
  int64_t duration;
  mtx_mp_rational_t frame_rate;
};

extern std::vector<common_frame_rate_t> g_common_frame_rates;

mtx_mp_rational_t determine_frame_rate(int64_t duration, int64_t max_difference = 20'000);

}
