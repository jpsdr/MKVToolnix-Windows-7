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

#include <stack>

class charset_converter_c;
class mm_io_c;

class mm_io_private_c {
public:
  bool dos_style_newlines{}, bom_written{};
  std::stack<int64_t> positions;
  int64_t current_position{}, cached_size{-1};
  std::shared_ptr<charset_converter_c> string_output_converter;

  virtual ~mm_io_private_c() = default;
};
