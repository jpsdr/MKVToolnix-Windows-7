/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the MPEG 4 part 10 ES video output module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/avc/es_parser.h"
#include "output/p_xyzvc_es.h"

class avc_es_video_packetizer_c: public xyzvc_es_video_packetizer_c {
protected:
  mtx::avc::es_parser_c &m_parser;

public:
  avc_es_video_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti, uint32_t width, uint32_t height);

  virtual translatable_string_c get_format_name() const override {
    return YT("AVC/H.264 (unframed)");
  };

  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message) override;

protected:
  virtual void check_if_default_duration_available() const override;
};
