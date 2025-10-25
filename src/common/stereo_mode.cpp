/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   the stereo mode helper

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/stereo_mode.h"
#include "common/strings/table_formatter.h"

#include <sstream>
#include <string>
#include <vector>

std::vector<std::string> stereo_mode_c::s_modes;
std::vector<translatable_string_c> stereo_mode_c::s_translations;

void
stereo_mode_c::init() {
  s_modes.emplace_back("mono");
  s_modes.emplace_back("side_by_side_left_first");
  s_modes.emplace_back("top_bottom_right_first");
  s_modes.emplace_back("top_bottom_left_first");
  s_modes.emplace_back("checkerboard_right_first");
  s_modes.emplace_back("checkerboard_left_first");
  s_modes.emplace_back("row_interleaved_right_first");
  s_modes.emplace_back("row_interleaved_left_first");
  s_modes.emplace_back("column_interleaved_right_first");
  s_modes.emplace_back("column_interleaved_left_first");
  s_modes.emplace_back("anaglyph_cyan_red");
  s_modes.emplace_back("side_by_side_right_first");
  s_modes.emplace_back("anaglyph_green_magenta");
  s_modes.emplace_back("both_eyes_laced_left_first");
  s_modes.emplace_back("both_eyes_laced_right_first");
}

void
stereo_mode_c::init_translations() {
  if (!s_translations.empty())
    return;

  s_translations.emplace_back(YT("mono"));
  s_translations.emplace_back(YT("side by side (left first)"));
  s_translations.emplace_back(YT("top bottom (right first)"));
  s_translations.emplace_back(YT("top bottom (left first)"));
  s_translations.emplace_back(YT("checkerboard (right first)"));
  s_translations.emplace_back(YT("checkerboard (left first)"));
  s_translations.emplace_back(YT("row interleaved (right first)"));
  s_translations.emplace_back(YT("row interleaved (left first)"));
  s_translations.emplace_back(YT("column interleaved (right first)"));
  s_translations.emplace_back(YT("column interleaved (left first)"));
  s_translations.emplace_back(YT("anaglyph (cyan/red)"));
  s_translations.emplace_back(YT("side by side (right first)"));
  s_translations.emplace_back(YT("anaglyph (green/magenta)"));
  s_translations.emplace_back(YT("both eyes laced in one block (left first)"));
  s_translations.emplace_back(YT("both eyes laced in one block (right first)"));
}

const std::string
stereo_mode_c::translate(unsigned int mode) {
  init_translations();
  return mode < s_translations.size() ? s_translations[mode].get_translated() : Y("unknown");
}

stereo_mode_c::mode
stereo_mode_c::parse_mode(const std::string &mode) {
  unsigned int idx;
  for (idx = 0; s_modes.size() > idx; ++idx)
    if (s_modes[idx] == mode)
      return static_cast<stereo_mode_c::mode>(idx);
  return invalid;
}

const std::string
stereo_mode_c::displayable_modes_list() {
  std::stringstream keywords_str;
  for (auto &keyword : s_modes) {
    if (!keywords_str.str().empty())
      keywords_str << ", ";
    keywords_str << "'" << keyword << "'";
  }

  return keywords_str.str();
}

bool
stereo_mode_c::valid_index(int idx) {
  return (0 <= idx) && (static_cast<int>(s_modes.size()) > idx);
}

void
stereo_mode_c::list() {
  auto formatter = mtx::string::table_formatter_c{}
    .set_header({ Y("Number"), Y("Symbolic name"), Y("Description") })
    .set_alignment({ mtx::string::table_formatter_c::align_right });

  for (auto idx = 0u; idx < s_modes.size(); ++idx)
    formatter.add_row({ fmt::format("{}", idx), s_modes[idx],  translate(idx) });

  mxinfo(formatter.format());
}
