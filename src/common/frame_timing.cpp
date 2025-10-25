/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/debugging.h"
#include "common/frame_timing.h"

namespace mtx::frame_timing {

std::vector<common_frame_rate_t> g_common_frame_rates{
  { 1'000'000'000ll         /    120, {    120,     1 } }, // 120 fps
  { 1'000'000'000ll         /    100, {    100,     1 } }, // 100 fps
  { 1'000'000'000ll         /     50, {     50,     1 } }, //  50 fps
  { 1'000'000'000ll         /     48, {     48,     1 } }, //  48 fps
  { 1'000'000'000ll         /     24, {     24,     1 } }, //  24 fps
  { 1'000'000'000ll         /     25, {     25,     1 } }, //  25 fps
  { 1'000'000'000ll         /     60, {     60,     1 } }, //  60 fps
  { 1'000'000'000ll         /     30, {     30,     1 } }, //  30 fps
  { 1'000'000'000ll * 1'001 / 48'000, { 48'000, 1'001 } }, //  47.952 fps
  { 1'000'000'000ll * 1'001 / 24'000, { 24'000, 1'001 } }, //  23.976 fps
  { 1'000'000'000ll * 1'001 / 50'000, { 50'000, 1'001 } }, //  24.975 frames per second telecined PAL
  { 1'000'000'000ll * 1'001 / 60'000, { 60'000, 1'001 } }, //  59.94 fps
  { 1'000'000'000ll * 1'001 / 30'000, { 30'000, 1'001 } }, //  29.97 fps
};

mtx_mp_rational_t
determine_frame_rate(int64_t duration,
                     int64_t max_difference) {
  static debugging_option_c s_debug{"determine_frame_rate|fix_bitstream_timing_info"};

  // search in the common FPS list
  using common_frame_rate_diff_t = std::pair<int64_t, common_frame_rate_t>;
  auto potentials = std::vector<common_frame_rate_diff_t>{};

  for (auto const &common_frame_rate : g_common_frame_rates) {
    auto difference = std::abs(duration - common_frame_rate.duration);
    if (difference < max_difference)
      potentials.emplace_back(difference, common_frame_rate);
  }

  if (potentials.empty()) {
    mxdebug_if(s_debug, fmt::format("determine_frame_rate: duration {0} max_difference {1}: no match found\n", duration, max_difference));
    return {};
  }

  std::sort(potentials.begin(), potentials.end(), [](auto const &a, auto const &b) {
    return a.first < b.first;
  });

  mxdebug_if(s_debug,
             fmt::format("determine_frame_rate: duration {0} max_difference {1}: {2} match(es) found returning {3} Δ {4}\n",
                         duration, max_difference, potentials.size(), potentials[0].second.frame_rate, potentials[0].first));

  return potentials[0].second.frame_rate;
}

}
