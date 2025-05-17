/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/bcp47.h"
#include "extract/track_spec.h"

class options_c {
public:
  enum extraction_mode_e {
    em_unknown,
    em_attachments,
    em_chapters,
    em_cuesheet,
    em_tags,
    em_timestamps_v2,
    em_tracks,
    em_cues,
  };

  class mode_options_c {
  public:
    bool m_simple_chapter_format, m_no_track_tags;
    mtx::bcp47::language_c m_simple_chapter_language;
    extraction_mode_e m_extraction_mode;

    std::vector<track_spec_t> m_tracks;

    std::string m_output_file_name;

    mode_options_c();

    void dump(std::string const &prefix) const;
  };

  std::string m_file_name;
  kax_analyzer_c::parse_mode_e m_parse_mode;

  std::vector<mode_options_c> m_modes;

public:
  options_c();

  void dump() const;
  std::vector<mode_options_c>::iterator get_options_for_mode(extraction_mode_e mode);
  void merge_tracks_and_timestamps_targets();
};
