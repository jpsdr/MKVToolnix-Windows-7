/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the Theora video output module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "output/p_generic_video.h"

class theora_video_packetizer_c: public generic_video_packetizer_c {
public:
  theora_video_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti, int64_t default_duration, int width, int height);
  virtual void set_headers();

  virtual translatable_string_c get_format_name() const {
    return YT("Theora");
  }

protected:
  virtual void process_impl(packet_cptr const &packet) override;
  virtual void extract_aspect_ratio();
};
