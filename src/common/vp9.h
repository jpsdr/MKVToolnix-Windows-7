/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html
*/

#pragma once

#include "common/common_pch.h"

namespace mtx { namespace vp9 {

namespace {

unsigned int constexpr CS_RGB = 7;

}

struct header_data_t {
  unsigned int profile{};
  boost::optional<unsigned int> level;
  unsigned int bit_depth{};
  bool subsampling_x{}, subsampling_y{};
};

boost::optional<header_data_t> parse_header_data(memory_c const &mem);

}}
