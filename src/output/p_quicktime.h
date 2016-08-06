/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definition for the QuickTime video output module

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef MTX_OUTPUT_P_QUICKTIME_H
#define MTX_OUTPUT_P_QUICKTIME_H

#include "common/common_pch.h"

#include "merge/generic_packetizer.h"
#include "output/p_video_for_windows.h"

class quicktime_video_packetizer_c: public video_for_windows_packetizer_c {
public:
  quicktime_video_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti, int width, int height);

  virtual translatable_string_c get_format_name() const {
    return YT("QuickTime compatible video");
  }
  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message);
};

#endif // MTX_OUTPUT_P_QUICKTIME_H
