/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definition for the AV1 output module

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/av1.h"
#include "merge/generic_packetizer.h"

class av1_video_packetizer_c: public generic_packetizer_c {
protected:
  int64_t m_previous_timestamp{-1};
  mtx::av1::parser_c m_parser;

public:
  av1_video_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti);

  virtual int process(packet_cptr packet) override;

  virtual translatable_string_c get_format_name() const override {
    return YT("AV1");
  }

  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message) override;
  virtual bool is_compatible_with(output_compatibility_e compatibility) override;
};
