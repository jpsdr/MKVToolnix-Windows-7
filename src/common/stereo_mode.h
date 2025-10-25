/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the stereo mode helper

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/translation.h"

class stereo_mode_c {
public:
  static std::vector<std::string> s_modes;
  static std::vector<translatable_string_c> s_translations;

  enum mode {
    invalid                        = -1,
    unspecified                    = -1,
    mono                           =  0,
    side_by_side_left_first        =  1,
    top_bottom_right_first         =  2,
    top_bottom_left_first          =  3,
    checkerboard_right_first       =  4,
    checkerboard_left_first        =  5,
    row_interleaved_right_first    =  6,
    row_interleaved_left_first     =  7,
    column_interleaved_right_first =  8,
    column_interleaved_left_first  =  9,
    anaglyph                       = 10,
    anaglyph_cyran_red             = 10,
    side_by_side_right_first       = 11,
    anaglyph_green_magenta         = 12,
    both_eyes_laced_left_first     = 13,
    both_eyes_laced_right_first    = 14,
  };

  static void init();
  static void init_translations();
  static const std::string translate(unsigned int mode);
  static const std::string displayable_modes_list();
  static mode parse_mode(const std::string &str);
  static bool valid_index(int index);
  static unsigned int max_index() {
    return 14;
  };
  static void list();
};
