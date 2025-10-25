/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the WebVTT text subtitle packetizer

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "output/p_textsubs.h"

class webvtt_packetizer_c: public textsubs_packetizer_c {
public:
  webvtt_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti);
  virtual ~webvtt_packetizer_c();

  virtual translatable_string_c get_format_name() const override {
    return YT("WebVTT subtitles");
  }
  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message) override;

protected:
  virtual void process_impl(packet_cptr const &packet) override;
};
