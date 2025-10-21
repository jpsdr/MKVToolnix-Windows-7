/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/list_utils.h"
#include "extract/mkvextract.h"
#include "extract/options.h"

options_c::mode_options_c::mode_options_c()
  : m_simple_chapter_format{}
  , m_no_track_tags{}
  , m_extraction_mode{options_c::em_unknown}
{
}

void
options_c::mode_options_c::dump(std::string const &prefix)
  const {
  mxinfo(fmt::format("{0}simple chapter format:   {1}\n"
                     "{0}simple chapter language: {2}\n"
                     "{0}extraction mode:         {3}\n"
                     "{0}num track specs:         {4}\n",
                     prefix, m_simple_chapter_format, m_simple_chapter_language.get_closest_iso639_2_alpha_3_code(), static_cast<int>(m_extraction_mode), m_tracks.size()));


  for (auto idx = 0u; idx < m_tracks.size(); ++idx) {
    mxinfo(fmt::format("\n{0}track spec #{1}:\n", prefix, idx));
    m_tracks[idx].dump(prefix + "  ");
  }
}

options_c::options_c()
  : m_parse_mode(kax_analyzer_c::parse_mode_fast)
{
  m_modes.emplace_back();
}

void
options_c::dump()
  const {
  mxinfo(fmt::format("options dump:\n"
                     "  file name:  {0}\n"
                     "  parse mode: {1}\n"
                     "  num modes:  {2}\n",
                     m_file_name, m_parse_mode == kax_analyzer_c::parse_mode_full ? "full" : "fast", m_modes.size()));

  for (auto idx = 0u; idx < m_modes.size(); ++idx) {
    mxinfo(fmt::format("\n  mode #{0}:\n", idx));
    m_modes[idx].dump("    ");
  }
}

std::vector<options_c::mode_options_c>::iterator
options_c::get_options_for_mode(extraction_mode_e mode) {
  auto itr = std::find_if(m_modes.begin(), m_modes.end(), [mode](auto const &mode_options) {
    return (mode_options.m_extraction_mode == mode)
        && mtx::included_in(mode, em_attachments, em_cues, em_timestamps_v2, em_tracks);
  });

  if (itr != m_modes.end())
    return itr;

  m_modes.emplace_back();
  itr                    = m_modes.end() - 1;
  itr->m_extraction_mode = mode;

  return itr;
}

void
options_c::merge_tracks_and_timestamps_targets() {
  auto tracks_itr     = std::find_if(m_modes.begin(), m_modes.end(), [](auto const &mode) { return em_tracks        == mode.m_extraction_mode; });
  auto timestamps_itr = std::find_if(m_modes.begin(), m_modes.end(), [](auto const &mode) { return em_timestamps_v2 == mode.m_extraction_mode; });

  if (timestamps_itr == m_modes.end())
    return;

  for (auto &tspec : timestamps_itr->m_tracks)
    tspec.target_mode = track_spec_t::tm_timestamps;

  if (tracks_itr == m_modes.end())
    timestamps_itr->m_extraction_mode = em_tracks;

  else {
    std::copy(timestamps_itr->m_tracks.begin(), timestamps_itr->m_tracks.end(), std::back_inserter(tracks_itr->m_tracks));
    m_modes.erase(timestamps_itr);
  }
}
