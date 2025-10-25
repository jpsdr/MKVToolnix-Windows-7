/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   definitions for channel layouts

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx::channels {

static uint64_t const front_left             = 0b0000'0000'00000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0001ull;
static uint64_t const front_right            = 0b0000'0000'00000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0010ull;
static uint64_t const front_center           = 0b0000'0000'00000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0100ull;
static uint64_t const low_frequency          = 0b0000'0000'00000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'1000ull;
static uint64_t const back_left              = 0b0000'0000'00000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0001'0000ull;
static uint64_t const back_right             = 0b0000'0000'00000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0010'0000ull;
static uint64_t const front_left_of_center   = 0b0000'0000'00000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0100'0000ull;
static uint64_t const front_right_of_center  = 0b0000'0000'00000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'1000'0000ull;
static uint64_t const back_center            = 0b0000'0000'00000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0001'0000'0000ull;
static uint64_t const side_left              = 0b0000'0000'00000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0010'0000'0000ull;
static uint64_t const side_right             = 0b0000'0000'00000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0100'0000'0000ull;
static uint64_t const top_center             = 0b0000'0000'00000'0000'0000'0000'0000'0000'0000'0000'0000'0000'1000'0000'0000ull;
static uint64_t const top_front_left         = 0b0000'0000'00000'0000'0000'0000'0000'0000'0000'0000'0000'0001'0000'0000'0000ull;
static uint64_t const top_front_center       = 0b0000'0000'00000'0000'0000'0000'0000'0000'0000'0000'0000'0010'0000'0000'0000ull;
static uint64_t const top_front_right        = 0b0000'0000'00000'0000'0000'0000'0000'0000'0000'0000'0000'0100'0000'0000'0000ull;
static uint64_t const top_back_left          = 0b0000'0000'00000'0000'0000'0000'0000'0000'0000'0000'0000'1000'0000'0000'0000ull;
static uint64_t const top_back_center        = 0b0000'0000'00000'0000'0000'0000'0000'0000'0000'0000'0001'0000'0000'0000'0000ull;
static uint64_t const top_back_right         = 0b0000'0000'00000'0000'0000'0000'0000'0000'0000'0000'0010'0000'0000'0000'0000ull;
static uint64_t const stereo_left            = 0b0000'0000'00000'0000'0000'0000'0000'0000'0000'0000'0100'0000'0000'0000'0000ull;
static uint64_t const stereo_right           = 0b0000'0000'00000'0000'0000'0000'0000'0000'0000'0000'1000'0000'0000'0000'0000ull;
static uint64_t const wide_left              = 0b0000'0000'00000'0000'0000'0000'0000'0000'0000'0001'0000'0000'0000'0000'0000ull;
static uint64_t const wide_right             = 0b0000'0000'00000'0000'0000'0000'0000'0000'0000'0010'0000'0000'0000'0000'0000ull;
static uint64_t const surround_direct_left   = 0b0000'0000'00000'0000'0000'0000'0000'0000'0000'0100'0000'0000'0000'0000'0000ull;
static uint64_t const surround_direct_right  = 0b0000'0000'00000'0000'0000'0000'0000'0000'0000'1000'0000'0000'0000'0000'0000ull;
static uint64_t const low_frequency_2        = 0b0000'0000'00000'0000'0000'0000'0000'0000'0001'0000'0000'0000'0000'0000'0000ull;

} // namespace mtx::channels
