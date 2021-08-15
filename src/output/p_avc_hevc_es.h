/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the  HEVC ES video output module

*/

#pragma once

#include "common/common_pch.h"

#include "common/hevc/es_parser.h"
#include "merge/generic_packetizer.h"

class avc_hevc_es_video_packetizer_c: public generic_packetizer_c {
protected:
  int64_t m_default_duration_for_interlaced_content{-1};
  std::optional<int64_t> m_parser_default_duration_to_force;
  bool m_first_frame{true}, m_set_display_dimensions{false};
  debugging_option_c m_debug_timestamps, m_debug_aspect_ratio;

public:
  avc_hevc_es_video_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti, std::string const &p_debug_type);

  virtual void set_headers() override;
};
