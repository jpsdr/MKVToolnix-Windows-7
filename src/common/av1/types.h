/*
  mkvmerge -- utility for splicing together matroska files
  from component media subtypes

  Distributed under the GPL v2
  see the file COPYING for details
  or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

  AV1 parser types

  Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx::av1 {

struct color_config_t {
  bool high_bitdepth{};
  bool twelve_bit{};
  bool mono_chrome{};
  uint8_t color_primaries{};
  uint8_t transfer_characteristics{};
  uint8_t matrix_coefficients{};
  bool video_full_range_flag{}; // color_range
  bool chroma_subsampling_x{};
  bool chroma_subsampling_y{};
  uint8_t chroma_sample_position{};
};

}                              // namespace mtx::av1
