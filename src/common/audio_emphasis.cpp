/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   the audio emphasis helper

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/audio_emphasis.h"
#include "common/strings/formatting.h"
#include "common/strings/table_formatter.h"

std::vector<std::string> audio_emphasis_c::s_modes;
std::vector<translatable_string_c> audio_emphasis_c::s_translations;

void
audio_emphasis_c::init() {
  s_modes.emplace_back("none");
  s_modes.emplace_back("cd_audio");
  s_modes.emplace_back("");
  s_modes.emplace_back("ccit_j_17");
  s_modes.emplace_back("fm_50");
  s_modes.emplace_back("fm_75");
  s_modes.emplace_back("");
  s_modes.emplace_back("");
  s_modes.emplace_back("");
  s_modes.emplace_back("");
  s_modes.emplace_back("phono_riaa");
  s_modes.emplace_back("phono_iec_n78");
  s_modes.emplace_back("phono_teldec");
  s_modes.emplace_back("phono_emi");
  s_modes.emplace_back("phono_columbia_lp");
  s_modes.emplace_back("phono_london");
  s_modes.emplace_back("phono_nartb");
}

void
audio_emphasis_c::init_translations() {
  if (!s_translations.empty())
    return;

  s_translations.emplace_back(YT("no emphasis"));
  s_translations.emplace_back(YT("first order filter found in CD/DVD/MPEG audio"));
  s_translations.emplace_back(YT("unknown"));
  s_translations.emplace_back(YT("CCIT-J.17"));
  s_translations.emplace_back(YT("FM radio in Europe"));
  s_translations.emplace_back(YT("FM radio in the USA"));
  s_translations.emplace_back(YT("unknown"));
  s_translations.emplace_back(YT("unknown"));
  s_translations.emplace_back(YT("unknown"));
  s_translations.emplace_back(YT("unknown"));
  s_translations.emplace_back(YT("phono filter (RIAA)"));
  s_translations.emplace_back(YT("phono filter (IEC N78)"));
  s_translations.emplace_back(YT("phono filter (Teldec)"));
  s_translations.emplace_back(YT("phono filter (EMI)"));
  s_translations.emplace_back(YT("phono filter (Columbia LP)"));
  s_translations.emplace_back(YT("phono filter (London)"));
  s_translations.emplace_back(YT("phono filter (NARTB)"));
}

std::string const
audio_emphasis_c::symbol(unsigned int mode) {
  return mode < s_modes.size() ? s_modes[mode] : std::string{};
}

std::string const
audio_emphasis_c::translate(unsigned int mode) {
  init_translations();
  return mode < s_translations.size() ? s_translations[mode].get_translated() : Y("unknown");
}

audio_emphasis_c::mode_e
audio_emphasis_c::parse_mode(const std::string &mode) {
  if (mode.empty())
    return invalid;

  unsigned int idx;
  for (idx = 0; s_modes.size() > idx; ++idx)
    if (s_modes[idx] == mode)
      return static_cast<audio_emphasis_c::mode_e>(idx);

  return invalid;
}

const std::string
audio_emphasis_c::displayable_modes_list() {
  std::vector<std::string> keywords;

  auto idx = 0;
  for (auto const &keyword : s_modes) {
    if (!keyword.empty())
      keywords.emplace_back(fmt::format("{0} ({1})", keyword, idx));
    ++idx;
  }

  return mtx::string::join(keywords, ", ");
}

bool
audio_emphasis_c::valid_index(int idx) {
  return (0 <= idx) && (static_cast<int>(s_modes.size()) > idx) && !s_modes[idx].empty();
}

void
audio_emphasis_c::list() {
  auto formatter = mtx::string::table_formatter_c{}
    .set_header({ Y("Number"), Y("Symbolic name"), Y("Description") })
    .set_alignment({ mtx::string::table_formatter_c::align_right });

  for (auto idx = 0u; idx < s_modes.size(); ++idx)
    if (valid_index(idx))
      formatter.add_row({ fmt::format("{}", idx), s_modes[idx],  translate(idx) });

  mxinfo(formatter.format());
}

int
audio_emphasis_c::max_index() {
  return 16;
}
