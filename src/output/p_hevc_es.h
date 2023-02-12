/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the HEVC ES video output module

*/

#pragma once

#include "common/common_pch.h"

#include "common/hevc/es_parser.h"
#include "output/p_avc_hevc_es.h"

class hevc_es_video_packetizer_c: public avc_hevc_es_video_packetizer_c {
protected:
  mtx::hevc::es_parser_c &m_parser;

public:
  hevc_es_video_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti, uint32_t width, uint32_t height);

  virtual translatable_string_c get_format_name() const override {
    return YT("HEVC/H.265 (unframed)");
  };

  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message) override;
};
