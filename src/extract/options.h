/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

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

  std::string m_file_name;
  bool m_simple_chapter_format;
  boost::optional<std::string> m_simple_chapter_language;
  kax_analyzer_c::parse_mode_e m_parse_mode;
  extraction_mode_e m_extraction_mode;

  std::vector<track_spec_t> m_tracks;

public:
  options_c();
};
