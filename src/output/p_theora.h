/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definition for the Theora video output module

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "output/p_generic_video.h"

class theora_video_packetizer_c: public generic_video_packetizer_c {
public:
  theora_video_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti, double fps, int width, int height);
  virtual void set_headers();
  virtual int process(packet_cptr packet);

  virtual translatable_string_c get_format_name() const {
    return YT("Theora");
  }

protected:
  virtual void extract_aspect_ratio();
};
