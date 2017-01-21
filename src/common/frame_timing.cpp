/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/frame_timing.h"

namespace mtx { namespace frame_timing {

std::vector<common_frame_rate_t> g_common_frame_rates{
  { 1000000000ll / 120,          {   120,    1 } }, // 120 fps
  { 1000000000ll / 100,          {   100,    1 } }, // 100 fps
  { 1000000000ll /  50,          {    50,    1 } }, //  50 fps
  { 1000000000ll /  48,          {    48,    1 } }, //  48 fps
  { 1000000000ll /  24,          {    24,    1 } }, //  24 fps
  { 1000000000ll /  25,          {    25,    1 } }, //  25 fps
  { 1000000000ll /  60,          {    60,    1 } }, //  60 fps
  { 1000000000ll /  30,          {    30,    1 } }, //  30 fps
  { 1000000000ll * 1001 / 48000, { 48000, 1001 } }, //  47.952 fps
  { 1000000000ll * 1001 / 24000, { 24000, 1001 } }, //  23.976 fps
  { 1000000000ll * 1001 / 50000, { 50000, 1001 } }, //  24.975 frames per second telecined PAL
  { 1000000000ll * 1001 / 60000, { 60000, 1001 } }, //  59.94 fps
  { 1000000000ll * 1001 / 30000, { 30000, 1001 } }, //  29.97 fps
};

int64_rational_c
determine_frame_rate(int64_t duration,
                     int64_t max_difference) {
  // search in the common FPS list
  using common_frame_rate_diff_t = std::pair<int64_t, common_frame_rate_t>;
  auto potentials = std::vector<common_frame_rate_diff_t>{};

  for (auto const &common_frame_rate : g_common_frame_rates) {
    auto difference = std::abs(duration - common_frame_rate.duration);
    if (difference < max_difference)
      potentials.emplace_back(difference, common_frame_rate);
  }

  if (potentials.empty())
    return {};

  brng::sort(potentials, [](auto const &a, auto const &b) {
    return a.first < b.first;
  });

  return potentials[0].second.frame_rate;
}

}}
