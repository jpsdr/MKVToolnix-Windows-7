/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definition for the WebVTT text subtitle packetizer

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef MTX_OUTPUT_P_WEBVTT_H
#define MTX_OUTPUT_P_WEBVTT_H

#include "common/common_pch.h"

#include "output/p_textsubs.h"

class webvtt_packetizer_c: public textsubs_packetizer_c {
public:
  webvtt_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti);
  virtual ~webvtt_packetizer_c();

  virtual int process(packet_cptr packet) override;

  virtual translatable_string_c get_format_name() const override {
    return YT("WebVTT subtitles");
  }
  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message) override;
};

#endif  // MTX_OUTPUT_P_WEBVTT_H
