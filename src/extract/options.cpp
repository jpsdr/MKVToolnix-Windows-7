/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/list_utils.h"
#include "extract/mkvextract.h"
#include "extract/options.h"

options_c::mode_options_c::mode_options_c()
  : m_simple_chapter_format{}
  , m_extraction_mode{options_c::em_unknown}
{
}

void
options_c::mode_options_c::dump(std::string const &prefix)
  const {
  mxinfo(boost::format("%1%simple chapter format:   %2%\n"
                       "%1%simple chapter language: %3%\n"
                       "%1%extraction mode:         %4%\n"
                       "%1%num track specs:         %5%\n")
         % prefix % m_simple_chapter_format % (m_simple_chapter_language ? *m_simple_chapter_language : std::string{"<none>"}) % static_cast<int>(m_extraction_mode) % m_tracks.size());


  for (auto idx = 0u; idx < m_tracks.size(); ++idx) {
    mxinfo(boost::format("\n%1%track spec #%2%:\n") % prefix % idx);
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
  mxinfo(boost::format("options dump:\n"
                       "  file name:  %1%\n"
                       "  parse mode: %2%\n"
                       "  num modes:  %3%\n")
         % m_file_name % (m_parse_mode == kax_analyzer_c::parse_mode_full ? "full" : "fast") % m_modes.size());

  for (auto idx = 0u; idx < m_modes.size(); ++idx) {
    mxinfo(boost::format("\n  mode #%1%:\n") % idx);
    m_modes[idx].dump("    ");
  }
}

std::vector<options_c::mode_options_c>::iterator
options_c::get_options_for_mode(extraction_mode_e mode) {
  auto itr = brng::find_if(m_modes, [mode](auto const &mode_options) {
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
  auto tracks_itr     = brng::find_if(m_modes, [](auto const &mode) { return em_tracks        == mode.m_extraction_mode; });
  auto timestamps_itr = brng::find_if(m_modes, [](auto const &mode) { return em_timestamps_v2 == mode.m_extraction_mode; });

  if (timestamps_itr == m_modes.end())
    return;

  for (auto &tspec : timestamps_itr->m_tracks)
    tspec.target_mode = track_spec_t::tm_timestamps;

  if (tracks_itr == m_modes.end())
    timestamps_itr->m_extraction_mode = em_tracks;

  else {
    brng::copy(timestamps_itr->m_tracks, std::back_inserter(tracks_itr->m_tracks));
    m_modes.erase(timestamps_itr);
  }
}
